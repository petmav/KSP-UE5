using System;
using System.Globalization;
using System.IO;
using System.Net.Security;
using System.Security.Cryptography.X509Certificates;
using System.Text.RegularExpressions;
using Mono.Security.Interface;
using Mono.Security.X509;
using Mono.Security.X509.Extensions;

namespace Mono.Net.Security;

internal static class SystemCertificateValidator
{
	private static bool is_macosx;

	private static X509RevocationMode revocation_mode;

	private static X509KeyUsageFlags s_flags;

	static SystemCertificateValidator()
	{
		s_flags = X509KeyUsageFlags.KeyAgreement | X509KeyUsageFlags.KeyEncipherment | X509KeyUsageFlags.DigitalSignature;
		is_macosx = Environment.OSVersion.Platform != PlatformID.Win32NT && File.Exists("/System/Library/Frameworks/Security.framework/Security");
		revocation_mode = X509RevocationMode.NoCheck;
		try
		{
			string environmentVariable = Environment.GetEnvironmentVariable("MONO_X509_REVOCATION_MODE");
			if (!string.IsNullOrEmpty(environmentVariable))
			{
				revocation_mode = (X509RevocationMode)Enum.Parse(typeof(X509RevocationMode), environmentVariable, ignoreCase: true);
			}
		}
		catch
		{
		}
	}

	public static System.Security.Cryptography.X509Certificates.X509Chain CreateX509Chain(System.Security.Cryptography.X509Certificates.X509CertificateCollection certs)
	{
		System.Security.Cryptography.X509Certificates.X509Chain x509Chain = new System.Security.Cryptography.X509Certificates.X509Chain();
		x509Chain.ChainPolicy = new X509ChainPolicy(certs);
		x509Chain.ChainPolicy.RevocationMode = revocation_mode;
		return x509Chain;
	}

	private static bool BuildX509Chain(System.Security.Cryptography.X509Certificates.X509CertificateCollection certs, System.Security.Cryptography.X509Certificates.X509Chain chain, ref SslPolicyErrors errors, ref int status11)
	{
		if (is_macosx)
		{
			return false;
		}
		X509Certificate2 certificate = (X509Certificate2)certs[0];
		bool flag;
		try
		{
			flag = chain.Build(certificate);
			if (!flag)
			{
				errors |= GetErrorsFromChain(chain);
			}
		}
		catch (Exception arg)
		{
			Console.Error.WriteLine("ERROR building certificate chain: {0}", arg);
			Console.Error.WriteLine("Please, report this problem to the Mono team");
			errors |= SslPolicyErrors.RemoteCertificateChainErrors;
			flag = false;
		}
		try
		{
			status11 = GetStatusFromChain(chain);
		}
		catch
		{
			status11 = -2146762485;
		}
		return flag;
	}

	private static bool CheckUsage(System.Security.Cryptography.X509Certificates.X509CertificateCollection certs, string host, ref SslPolicyErrors errors, ref int status11)
	{
		X509Certificate2 x509Certificate = certs[0] as X509Certificate2;
		if (x509Certificate == null)
		{
			x509Certificate = new X509Certificate2(certs[0]);
		}
		if (!is_macosx)
		{
			if (!CheckCertificateUsage(x509Certificate))
			{
				errors |= SslPolicyErrors.RemoteCertificateChainErrors;
				status11 = -2146762490;
				return false;
			}
			if (!string.IsNullOrEmpty(host) && !CheckServerIdentity(x509Certificate, host))
			{
				errors |= SslPolicyErrors.RemoteCertificateNameMismatch;
				status11 = -2146762481;
				return false;
			}
		}
		return true;
	}

	private static bool EvaluateSystem(System.Security.Cryptography.X509Certificates.X509CertificateCollection certs, System.Security.Cryptography.X509Certificates.X509CertificateCollection anchors, string host, System.Security.Cryptography.X509Certificates.X509Chain chain, ref SslPolicyErrors errors, ref int status11)
	{
		_ = certs[0];
		bool flag;
		if (is_macosx)
		{
			OSX509Certificates.SecTrustResult secTrustResult = OSX509Certificates.SecTrustResult.Deny;
			try
			{
				secTrustResult = OSX509Certificates.TrustEvaluateSsl(certs, anchors, host);
				flag = secTrustResult == OSX509Certificates.SecTrustResult.Proceed || secTrustResult == OSX509Certificates.SecTrustResult.Unspecified;
			}
			catch
			{
				flag = false;
				errors |= SslPolicyErrors.RemoteCertificateChainErrors;
			}
			if (flag)
			{
				errors = SslPolicyErrors.None;
			}
			else
			{
				status11 = (int)secTrustResult;
				errors |= SslPolicyErrors.RemoteCertificateChainErrors;
			}
		}
		else
		{
			flag = BuildX509Chain(certs, chain, ref errors, ref status11);
		}
		return flag;
	}

	public static bool Evaluate(MonoTlsSettings settings, string host, System.Security.Cryptography.X509Certificates.X509CertificateCollection certs, System.Security.Cryptography.X509Certificates.X509Chain chain, ref SslPolicyErrors errors, ref int status11)
	{
		if (!CheckUsage(certs, host, ref errors, ref status11))
		{
			return false;
		}
		if (settings != null && settings.SkipSystemValidators)
		{
			return false;
		}
		System.Security.Cryptography.X509Certificates.X509CertificateCollection anchors = settings?.TrustAnchors;
		return EvaluateSystem(certs, anchors, host, chain, ref errors, ref status11);
	}

	internal static bool NeedsChain(MonoTlsSettings settings)
	{
		if (!is_macosx)
		{
			return true;
		}
		if (!CertificateValidationHelper.SupportsX509Chain)
		{
			return false;
		}
		if (settings != null)
		{
			if (settings.SkipSystemValidators)
			{
				return settings.CallbackNeedsCertificateChain;
			}
			return true;
		}
		return true;
	}

	private static int GetStatusFromChain(System.Security.Cryptography.X509Certificates.X509Chain chain)
	{
		long num = 0L;
		X509ChainStatus[] chainStatus = chain.ChainStatus;
		foreach (X509ChainStatus x509ChainStatus in chainStatus)
		{
			System.Security.Cryptography.X509Certificates.X509ChainStatusFlags status = x509ChainStatus.Status;
			if (status != 0)
			{
				num = (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.NotTimeValid) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.NotTimeNested) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.Revoked) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.NotSignatureValid) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.NotValidForUsage) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.UntrustedRoot) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.RevocationStatusUnknown) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.Cyclic) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.InvalidExtension) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.InvalidPolicyConstraints) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.InvalidBasicConstraints) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.InvalidNameConstraints) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.HasNotSupportedNameConstraint) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.HasNotDefinedNameConstraint) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.HasNotPermittedNameConstraint) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.HasExcludedNameConstraint) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.PartialChain) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.CtlNotTimeValid) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.CtlNotSignatureValid) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.CtlNotValidForUsage) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.OfflineRevocation) == 0) ? (((status & System.Security.Cryptography.X509Certificates.X509ChainStatusFlags.NoIssuanceChainPolicy) == 0) ? 2148204811u : 2148204807u) : 2148081682u) : 2148204816u) : 2148098052u) : 2148204801u) : 2148204810u) : 2148204820u) : 2148204820u) : 2148204820u) : 2148204820u) : 2148204820u) : 2148098073u) : 2148204813u) : 2148204811u) : 2148204810u) : 2148081682u) : 2148204809u) : 2148204816u) : 2148098052u) : 2148204812u) : 2148204802u) : 2148204801u);
				break;
			}
		}
		return (int)num;
	}

	private static SslPolicyErrors GetErrorsFromChain(System.Security.Cryptography.X509Certificates.X509Chain chain)
	{
		SslPolicyErrors sslPolicyErrors = SslPolicyErrors.None;
		X509ChainStatus[] chainStatus = chain.ChainStatus;
		foreach (X509ChainStatus x509ChainStatus in chainStatus)
		{
			if (x509ChainStatus.Status != 0)
			{
				sslPolicyErrors |= SslPolicyErrors.RemoteCertificateChainErrors;
				break;
			}
		}
		return sslPolicyErrors;
	}

	private static bool CheckCertificateUsage(X509Certificate2 cert)
	{
		try
		{
			if (cert.Version < 3)
			{
				return true;
			}
			X509KeyUsageExtension x509KeyUsageExtension = cert.Extensions["2.5.29.15"] as X509KeyUsageExtension;
			X509EnhancedKeyUsageExtension x509EnhancedKeyUsageExtension = cert.Extensions["2.5.29.37"] as X509EnhancedKeyUsageExtension;
			if (x509KeyUsageExtension != null && x509EnhancedKeyUsageExtension != null)
			{
				if ((x509KeyUsageExtension.KeyUsages & s_flags) == 0)
				{
					return false;
				}
				return x509EnhancedKeyUsageExtension.EnhancedKeyUsages["1.3.6.1.5.5.7.3.1"] != null || x509EnhancedKeyUsageExtension.EnhancedKeyUsages["2.16.840.1.113730.4.1"] != null;
			}
			if (x509KeyUsageExtension != null)
			{
				return (x509KeyUsageExtension.KeyUsages & s_flags) != 0;
			}
			if (x509EnhancedKeyUsageExtension != null)
			{
				return x509EnhancedKeyUsageExtension.EnhancedKeyUsages["1.3.6.1.5.5.7.3.1"] != null || x509EnhancedKeyUsageExtension.EnhancedKeyUsages["2.16.840.1.113730.4.1"] != null;
			}
			System.Security.Cryptography.X509Certificates.X509Extension x509Extension = cert.Extensions["2.16.840.1.113730.1.1"];
			if (x509Extension != null)
			{
				return x509Extension.NetscapeCertType(multiLine: false).IndexOf("SSL Server Authentication", StringComparison.Ordinal) != -1;
			}
			return true;
		}
		catch (Exception arg)
		{
			Console.Error.WriteLine("ERROR processing certificate: {0}", arg);
			Console.Error.WriteLine("Please, report this problem to the Mono team");
			return false;
		}
	}

	private static bool CheckServerIdentity(X509Certificate2 cert, string targetHost)
	{
		try
		{
			Mono.Security.X509.X509Certificate x509Certificate = new Mono.Security.X509.X509Certificate(cert.RawData);
			Mono.Security.X509.X509Extension x509Extension = x509Certificate.Extensions["2.5.29.17"];
			if (x509Extension != null)
			{
				SubjectAltNameExtension subjectAltNameExtension = new SubjectAltNameExtension(x509Extension);
				string[] dNSNames = subjectAltNameExtension.DNSNames;
				foreach (string pattern in dNSNames)
				{
					if (Match(targetHost, pattern))
					{
						return true;
					}
				}
				dNSNames = subjectAltNameExtension.IPAddresses;
				for (int i = 0; i < dNSNames.Length; i++)
				{
					if (dNSNames[i] == targetHost)
					{
						return true;
					}
				}
			}
			return CheckDomainName(x509Certificate.SubjectName, targetHost);
		}
		catch (Exception arg)
		{
			Console.Error.WriteLine("ERROR processing certificate: {0}", arg);
			Console.Error.WriteLine("Please, report this problem to the Mono team");
			return false;
		}
	}

	private static bool CheckDomainName(string subjectName, string targetHost)
	{
		string pattern = string.Empty;
		MatchCollection matchCollection = new Regex("CN\\s*=\\s*([^,]*)").Matches(subjectName);
		if (matchCollection.Count == 1 && matchCollection[0].Success)
		{
			pattern = matchCollection[0].Groups[1].Value.ToString();
		}
		return Match(targetHost, pattern);
	}

	private static bool Match(string hostname, string pattern)
	{
		int num = pattern.IndexOf('*');
		if (num == -1)
		{
			return string.Compare(hostname, pattern, ignoreCase: true, CultureInfo.InvariantCulture) == 0;
		}
		if (num != pattern.Length - 1 && pattern[num + 1] != '.')
		{
			return false;
		}
		if (pattern.IndexOf('*', num + 1) != -1)
		{
			return false;
		}
		string text = pattern.Substring(num + 1);
		int num2 = hostname.Length - text.Length;
		if (num2 <= 0)
		{
			return false;
		}
		if (string.Compare(hostname, num2, text, 0, text.Length, ignoreCase: true, CultureInfo.InvariantCulture) != 0)
		{
			return false;
		}
		if (num == 0)
		{
			int num3 = hostname.IndexOf('.');
			if (num3 != -1)
			{
				return num3 >= hostname.Length - text.Length;
			}
			return true;
		}
		string text2 = pattern.Substring(0, num);
		return string.Compare(hostname, 0, text2, 0, text2.Length, ignoreCase: true, CultureInfo.InvariantCulture) == 0;
	}
}

using System;
using System.Security.Cryptography.X509Certificates;
using Mono.Security.Interface;

namespace Mono.AppleTls;

internal static class AppleCertificateHelper
{
	public static SecIdentity GetIdentity(X509Certificate certificate)
	{
		if (certificate is X509Certificate2 certificate2)
		{
			return SecImportExport.ItemImport(certificate2);
		}
		return null;
	}

	public static SecIdentity GetIdentity(X509Certificate certificate, out SecCertificate[] intermediateCerts)
	{
		SecIdentity identity = GetIdentity(certificate);
		if (!(certificate.Impl is X509Certificate2Impl { IntermediateCertificates: not null } x509Certificate2Impl))
		{
			intermediateCerts = new SecCertificate[0];
			return identity;
		}
		try
		{
			intermediateCerts = new SecCertificate[x509Certificate2Impl.IntermediateCertificates.Count];
			for (int i = 0; i < intermediateCerts.Length; i++)
			{
				intermediateCerts[i] = new SecCertificate(x509Certificate2Impl.IntermediateCertificates[i]);
			}
			return identity;
		}
		catch
		{
			identity.Dispose();
			throw;
		}
	}

	public static bool InvokeSystemCertificateValidator(ICertificateValidator2 validator, string targetHost, bool serverMode, X509CertificateCollection certificates, ref MonoSslPolicyErrors errors, ref int status11)
	{
		if (certificates == null)
		{
			errors |= MonoSslPolicyErrors.RemoteCertificateNotAvailable;
			return false;
		}
		if (!string.IsNullOrEmpty(targetHost))
		{
			int num = targetHost.IndexOf(':');
			if (num > 0)
			{
				targetHost = targetHost.Substring(0, num);
			}
		}
		using SecPolicy policy = SecPolicy.CreateSslPolicy(!serverMode, targetHost);
		using SecTrust secTrust = new SecTrust(certificates, policy);
		if (validator.Settings.TrustAnchors != null)
		{
			SecStatusCode secStatusCode = secTrust.SetAnchorCertificates(validator.Settings.TrustAnchors);
			if (secStatusCode != 0)
			{
				throw new InvalidOperationException(secStatusCode.ToString());
			}
			secTrust.SetAnchorCertificatesOnly(anchorCertificatesOnly: false);
		}
		if (validator.Settings.CertificateValidationTime.HasValue)
		{
			SecStatusCode secStatusCode2 = secTrust.SetVerifyDate(validator.Settings.CertificateValidationTime.Value);
			if (secStatusCode2 != 0)
			{
				throw new InvalidOperationException(secStatusCode2.ToString());
			}
		}
		SecTrustResult secTrustResult = secTrust.Evaluate();
		if (secTrustResult == SecTrustResult.Unspecified || secTrustResult == SecTrustResult.Proceed)
		{
			return true;
		}
		errors |= MonoSslPolicyErrors.RemoteCertificateChainErrors;
		return false;
	}
}

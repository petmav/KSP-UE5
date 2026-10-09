using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Text;
using System.Threading;
using Mono.Security.X509;
using XamMac.CoreFoundation;

namespace System.Security.Cryptography.X509Certificates;

internal static class X509Helper
{
	internal struct CertificateContext
	{
		public uint dwCertEncodingType;

		public IntPtr pbCertEncoded;

		public uint cbCertEncoded;

		public IntPtr pCertInfo;

		public IntPtr hCertStore;
	}

	private static INativeCertificateHelper nativeHelper;

	private static bool ShouldUseAppleTls
	{
		get
		{
			if (!Environment.IsMacOS)
			{
				return false;
			}
			string environmentVariable = Environment.GetEnvironmentVariable("MONO_TLS_PROVIDER");
			if (!string.IsNullOrEmpty(environmentVariable) && !(environmentVariable == "default"))
			{
				return environmentVariable == "apple";
			}
			return true;
		}
	}

	public static X509CertificateImpl InitFromHandleApple(IntPtr handle)
	{
		return new X509CertificateImplApple(handle, owns: false);
	}

	private static X509CertificateImpl ImportApple(byte[] rawData)
	{
		IntPtr intPtr = CFHelpers.CreateCertificateFromData(rawData);
		if (intPtr != IntPtr.Zero)
		{
			return new X509CertificateImplApple(intPtr, owns: true);
		}
		Mono.Security.X509.X509Certificate x;
		try
		{
			x = new Mono.Security.X509.X509Certificate(rawData);
		}
		catch (Exception inner)
		{
			try
			{
				x = ImportPkcs12(rawData, null);
			}
			catch
			{
				throw new CryptographicException(Locale.GetText("Unable to decode certificate."), inner);
			}
		}
		return new X509CertificateImplMono(x);
	}

	internal static void InstallNativeHelper(INativeCertificateHelper helper)
	{
		if (nativeHelper == null)
		{
			Interlocked.CompareExchange(ref nativeHelper, helper, null);
		}
	}

	public static X509CertificateImpl InitFromHandle(IntPtr handle)
	{
		if (ShouldUseAppleTls)
		{
			return InitFromHandleApple(handle);
		}
		return InitFromHandleCore(handle);
	}

	private static X509CertificateImpl Import(byte[] rawData)
	{
		if (ShouldUseAppleTls)
		{
			return ImportApple(rawData);
		}
		return ImportCore(rawData);
	}

	[SecurityPermission(SecurityAction.Demand, UnmanagedCode = true)]
	public static X509CertificateImpl InitFromHandleCore(IntPtr handle)
	{
		CertificateContext certificateContext = (CertificateContext)Marshal.PtrToStructure(handle, typeof(CertificateContext));
		byte[] array = new byte[certificateContext.cbCertEncoded];
		Marshal.Copy(certificateContext.pbCertEncoded, array, 0, (int)certificateContext.cbCertEncoded);
		return new X509CertificateImplMono(new Mono.Security.X509.X509Certificate(array));
	}

	public static X509CertificateImpl InitFromCertificate(X509Certificate cert)
	{
		if (nativeHelper != null)
		{
			return nativeHelper.Import(cert);
		}
		return InitFromCertificate(cert.Impl);
	}

	public static X509CertificateImpl InitFromCertificate(X509CertificateImpl impl)
	{
		ThrowIfContextInvalid(impl);
		X509CertificateImpl x509CertificateImpl = impl.Clone();
		if (x509CertificateImpl != null)
		{
			return x509CertificateImpl;
		}
		byte[] rawCertData = impl.GetRawCertData();
		if (rawCertData == null)
		{
			return null;
		}
		return new X509CertificateImplMono(new Mono.Security.X509.X509Certificate(rawCertData));
	}

	public static bool IsValid(X509CertificateImpl impl)
	{
		return impl?.IsValid ?? false;
	}

	internal static void ThrowIfContextInvalid(X509CertificateImpl impl)
	{
		if (!IsValid(impl))
		{
			throw GetInvalidContextException();
		}
	}

	internal static Exception GetInvalidContextException()
	{
		return new CryptographicException(Locale.GetText("Certificate instance is empty."));
	}

	internal static Mono.Security.X509.X509Certificate ImportPkcs12(byte[] rawData, string password)
	{
		PKCS12 pKCS = ((password == null) ? new PKCS12(rawData) : new PKCS12(rawData, password));
		if (pKCS.Certificates.Count == 0)
		{
			return null;
		}
		if (pKCS.Keys.Count == 0)
		{
			return pKCS.Certificates[0];
		}
		string text = (pKCS.Keys[0] as AsymmetricAlgorithm).ToXmlString(includePrivateParameters: false);
		foreach (Mono.Security.X509.X509Certificate certificate in pKCS.Certificates)
		{
			if (certificate.RSA != null && text == certificate.RSA.ToXmlString(includePrivateParameters: false))
			{
				return certificate;
			}
			if (certificate.DSA != null && text == certificate.DSA.ToXmlString(includePrivateParameters: false))
			{
				return certificate;
			}
		}
		return pKCS.Certificates[0];
	}

	private static byte[] PEM(string type, byte[] data)
	{
		string @string = Encoding.ASCII.GetString(data);
		string text = $"-----BEGIN {type}-----";
		string value = $"-----END {type}-----";
		int num = @string.IndexOf(text) + text.Length;
		int num2 = @string.IndexOf(value, num);
		return Convert.FromBase64String(@string.Substring(num, num2 - num));
	}

	private static byte[] ConvertData(byte[] data)
	{
		if (data == null || data.Length == 0)
		{
			return data;
		}
		if (data[0] != 48)
		{
			try
			{
				return PEM("CERTIFICATE", data);
			}
			catch
			{
			}
		}
		return data;
	}

	private static X509CertificateImpl ImportCore(byte[] rawData)
	{
		Mono.Security.X509.X509Certificate x;
		try
		{
			x = new Mono.Security.X509.X509Certificate(rawData);
		}
		catch (Exception inner)
		{
			try
			{
				x = ImportPkcs12(rawData, null);
			}
			catch
			{
				throw new CryptographicException(Locale.GetText("Unable to decode certificate."), inner);
			}
		}
		return new X509CertificateImplMono(x);
	}

	public static X509CertificateImpl Import(byte[] rawData, string password, X509KeyStorageFlags keyStorageFlags)
	{
		if (password == null)
		{
			rawData = ConvertData(rawData);
			return Import(rawData);
		}
		Mono.Security.X509.X509Certificate x;
		try
		{
			x = ImportPkcs12(rawData, password);
		}
		catch
		{
			x = new Mono.Security.X509.X509Certificate(rawData);
		}
		return new X509CertificateImplMono(x);
	}

	public static byte[] Export(X509CertificateImpl impl, X509ContentType contentType, byte[] password)
	{
		ThrowIfContextInvalid(impl);
		return impl.Export(contentType, password);
	}

	public static bool Equals(X509CertificateImpl first, X509CertificateImpl second)
	{
		if (!IsValid(first) || !IsValid(second))
		{
			return false;
		}
		if (first.Equals(second, out var result))
		{
			return result;
		}
		byte[] rawCertData = first.GetRawCertData();
		byte[] rawCertData2 = second.GetRawCertData();
		if (rawCertData == null)
		{
			return rawCertData2 == null;
		}
		if (rawCertData2 == null)
		{
			return false;
		}
		if (rawCertData.Length != rawCertData2.Length)
		{
			return false;
		}
		for (int i = 0; i < rawCertData.Length; i++)
		{
			if (rawCertData[i] != rawCertData2[i])
			{
				return false;
			}
		}
		return true;
	}

	public static string ToHexString(byte[] data)
	{
		if (data != null)
		{
			StringBuilder stringBuilder = new StringBuilder();
			for (int i = 0; i < data.Length; i++)
			{
				stringBuilder.Append(data[i].ToString("X2"));
			}
			return stringBuilder.ToString();
		}
		return null;
	}
}

using System.Runtime.InteropServices;
using System.Text;
using Mono.Security.X509;
using XamMac.CoreFoundation;

namespace System.Security.Cryptography.X509Certificates;

internal class X509CertificateImplApple : X509CertificateImpl
{
	private IntPtr handle;

	private X509CertificateImpl fallback;

	public override bool IsValid => handle != IntPtr.Zero;

	public override IntPtr Handle => handle;

	public X509CertificateImpl FallbackImpl
	{
		get
		{
			MustFallback();
			return fallback;
		}
	}

	public X509CertificateImplApple(IntPtr handle, bool owns)
	{
		this.handle = handle;
		if (!owns)
		{
			CFHelpers.CFRetain(handle);
		}
	}

	public override IntPtr GetNativeAppleCertificate()
	{
		ThrowIfContextInvalid();
		return handle;
	}

	public override X509CertificateImpl Clone()
	{
		ThrowIfContextInvalid();
		return new X509CertificateImplApple(handle, owns: false);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SecCertificateCopySubjectSummary(IntPtr cert);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SecCertificateCopyData(IntPtr cert);

	public override byte[] GetRawCertData()
	{
		ThrowIfContextInvalid();
		IntPtr intPtr = SecCertificateCopyData(handle);
		if (intPtr == IntPtr.Zero)
		{
			throw new ArgumentException("Not a valid certificate");
		}
		try
		{
			return CFHelpers.FetchDataBuffer(intPtr);
		}
		finally
		{
			CFHelpers.CFRelease(intPtr);
		}
	}

	public string GetSubjectSummary()
	{
		ThrowIfContextInvalid();
		IntPtr obj = SecCertificateCopySubjectSummary(handle);
		string result = CFHelpers.FetchString(obj);
		CFHelpers.CFRelease(obj);
		return result;
	}

	protected override byte[] GetCertHash(bool lazy)
	{
		ThrowIfContextInvalid();
		return SHA1.Create().ComputeHash(GetRawCertData());
	}

	public override bool Equals(X509CertificateImpl other, out bool result)
	{
		if (other is X509CertificateImplApple x509CertificateImplApple && x509CertificateImplApple.handle == handle)
		{
			result = true;
			return true;
		}
		result = false;
		return false;
	}

	private void MustFallback()
	{
		ThrowIfContextInvalid();
		if (fallback == null)
		{
			Mono.Security.X509.X509Certificate x = new Mono.Security.X509.X509Certificate(GetRawCertData());
			fallback = new X509CertificateImplMono(x);
		}
	}

	public override string GetSubjectName(bool legacyV1Mode)
	{
		return FallbackImpl.GetSubjectName(legacyV1Mode);
	}

	public override string GetIssuerName(bool legacyV1Mode)
	{
		return FallbackImpl.GetIssuerName(legacyV1Mode);
	}

	public override DateTime GetValidFrom()
	{
		return FallbackImpl.GetValidFrom();
	}

	public override DateTime GetValidUntil()
	{
		return FallbackImpl.GetValidUntil();
	}

	public override string GetKeyAlgorithm()
	{
		return FallbackImpl.GetKeyAlgorithm();
	}

	public override byte[] GetKeyAlgorithmParameters()
	{
		return FallbackImpl.GetKeyAlgorithmParameters();
	}

	public override byte[] GetPublicKey()
	{
		return FallbackImpl.GetPublicKey();
	}

	public override byte[] GetSerialNumber()
	{
		return FallbackImpl.GetSerialNumber();
	}

	public override byte[] Export(X509ContentType contentType, byte[] password)
	{
		ThrowIfContextInvalid();
		return contentType switch
		{
			X509ContentType.Cert => GetRawCertData(), 
			X509ContentType.Pfx => throw new NotSupportedException(), 
			X509ContentType.SerializedCert => throw new NotSupportedException(), 
			_ => throw new CryptographicException(Locale.GetText("This certificate format '{0}' cannot be exported.", contentType)), 
		};
	}

	public override string ToString(bool full)
	{
		ThrowIfContextInvalid();
		if (!full || fallback == null)
		{
			string subjectSummary = GetSubjectSummary();
			return $"[X509Certificate: {subjectSummary}]";
		}
		string newLine = Environment.NewLine;
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.AppendFormat("[Subject]{0}  {1}{0}{0}", newLine, GetSubjectName(legacyV1Mode: false));
		stringBuilder.AppendFormat("[Issuer]{0}  {1}{0}{0}", newLine, GetIssuerName(legacyV1Mode: false));
		stringBuilder.AppendFormat("[Not Before]{0}  {1}{0}{0}", newLine, GetValidFrom().ToLocalTime());
		stringBuilder.AppendFormat("[Not After]{0}  {1}{0}{0}", newLine, GetValidUntil().ToLocalTime());
		stringBuilder.AppendFormat("[Thumbprint]{0}  {1}{0}", newLine, X509Helper.ToHexString(GetCertHash()));
		stringBuilder.Append(newLine);
		return stringBuilder.ToString();
	}

	protected override void Dispose(bool disposing)
	{
		if (handle != IntPtr.Zero)
		{
			CFHelpers.CFRelease(handle);
			handle = IntPtr.Zero;
		}
		if (fallback != null)
		{
			fallback.Dispose();
			fallback = null;
		}
	}
}

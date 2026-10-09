using System.Text;
using Mono.Security.X509;

namespace System.Security.Cryptography.X509Certificates;

internal sealed class X509CertificateImplMono : X509CertificateImpl
{
	private Mono.Security.X509.X509Certificate x509;

	public override bool IsValid => x509 != null;

	public override IntPtr Handle => IntPtr.Zero;

	public X509CertificateImplMono(Mono.Security.X509.X509Certificate x509)
	{
		this.x509 = x509;
	}

	public override IntPtr GetNativeAppleCertificate()
	{
		return IntPtr.Zero;
	}

	public override X509CertificateImpl Clone()
	{
		ThrowIfContextInvalid();
		return new X509CertificateImplMono(x509);
	}

	public override string GetIssuerName(bool legacyV1Mode)
	{
		ThrowIfContextInvalid();
		if (legacyV1Mode)
		{
			return x509.IssuerName;
		}
		return X501.ToString(x509.GetIssuerName(), reversed: true, ", ", quotes: true);
	}

	public override string GetSubjectName(bool legacyV1Mode)
	{
		ThrowIfContextInvalid();
		if (legacyV1Mode)
		{
			return x509.SubjectName;
		}
		return X501.ToString(x509.GetSubjectName(), reversed: true, ", ", quotes: true);
	}

	public override byte[] GetRawCertData()
	{
		ThrowIfContextInvalid();
		return x509.RawData;
	}

	protected override byte[] GetCertHash(bool lazy)
	{
		ThrowIfContextInvalid();
		return SHA1.Create().ComputeHash(x509.RawData);
	}

	public override DateTime GetValidFrom()
	{
		ThrowIfContextInvalid();
		return x509.ValidFrom;
	}

	public override DateTime GetValidUntil()
	{
		ThrowIfContextInvalid();
		return x509.ValidUntil;
	}

	public override bool Equals(X509CertificateImpl other, out bool result)
	{
		result = false;
		return false;
	}

	public override string GetKeyAlgorithm()
	{
		ThrowIfContextInvalid();
		return x509.KeyAlgorithm;
	}

	public override byte[] GetKeyAlgorithmParameters()
	{
		ThrowIfContextInvalid();
		return x509.KeyAlgorithmParameters;
	}

	public override byte[] GetPublicKey()
	{
		ThrowIfContextInvalid();
		return x509.PublicKey;
	}

	public override byte[] GetSerialNumber()
	{
		ThrowIfContextInvalid();
		return x509.SerialNumber;
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
		x509 = null;
	}
}

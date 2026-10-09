namespace System.Security.Cryptography.X509Certificates;

internal abstract class X509CertificateImpl : IDisposable
{
	private byte[] cachedCertificateHash;

	public abstract bool IsValid { get; }

	public abstract IntPtr Handle { get; }

	public abstract IntPtr GetNativeAppleCertificate();

	protected void ThrowIfContextInvalid()
	{
		if (!IsValid)
		{
			throw X509Helper.GetInvalidContextException();
		}
	}

	public abstract X509CertificateImpl Clone();

	public abstract string GetIssuerName(bool legacyV1Mode);

	public abstract string GetSubjectName(bool legacyV1Mode);

	public abstract byte[] GetRawCertData();

	public abstract DateTime GetValidFrom();

	public abstract DateTime GetValidUntil();

	public byte[] GetCertHash()
	{
		ThrowIfContextInvalid();
		if (cachedCertificateHash == null)
		{
			cachedCertificateHash = GetCertHash(lazy: false);
		}
		return cachedCertificateHash;
	}

	protected abstract byte[] GetCertHash(bool lazy);

	public override int GetHashCode()
	{
		if (!IsValid)
		{
			return 0;
		}
		if (cachedCertificateHash == null)
		{
			cachedCertificateHash = GetCertHash(lazy: true);
		}
		if (cachedCertificateHash != null && cachedCertificateHash.Length >= 4)
		{
			return (cachedCertificateHash[0] << 24) | (cachedCertificateHash[1] << 16) | (cachedCertificateHash[2] << 8) | cachedCertificateHash[3];
		}
		return 0;
	}

	public abstract bool Equals(X509CertificateImpl other, out bool result);

	public abstract string GetKeyAlgorithm();

	public abstract byte[] GetKeyAlgorithmParameters();

	public abstract byte[] GetPublicKey();

	public abstract byte[] GetSerialNumber();

	public abstract byte[] Export(X509ContentType contentType, byte[] password);

	public abstract string ToString(bool full);

	public override bool Equals(object obj)
	{
		if (!(obj is X509CertificateImpl x509CertificateImpl))
		{
			return false;
		}
		if (!IsValid || !x509CertificateImpl.IsValid)
		{
			return false;
		}
		if (Equals(x509CertificateImpl, out var result))
		{
			return result;
		}
		byte[] rawCertData = GetRawCertData();
		byte[] rawCertData2 = x509CertificateImpl.GetRawCertData();
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

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	protected virtual void Dispose(bool disposing)
	{
		cachedCertificateHash = null;
	}

	~X509CertificateImpl()
	{
		Dispose(disposing: false);
	}
}

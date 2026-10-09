using System.Security.Cryptography.X509Certificates;
using Unity;

namespace System.Security.Cryptography.Pkcs;

public sealed class SignerInfo
{
	private SubjectIdentifier _signer;

	private X509Certificate2 _certificate;

	private Oid _digest;

	private SignerInfoCollection _counter;

	private CryptographicAttributeObjectCollection _signed;

	private CryptographicAttributeObjectCollection _unsigned;

	private int _version;

	public CryptographicAttributeObjectCollection SignedAttributes => _signed;

	public X509Certificate2 Certificate => _certificate;

	public SignerInfoCollection CounterSignerInfos => _counter;

	public Oid DigestAlgorithm => _digest;

	public SubjectIdentifier SignerIdentifier => _signer;

	public CryptographicAttributeObjectCollection UnsignedAttributes => _unsigned;

	public int Version => _version;

	internal SignerInfo(string hashName, X509Certificate2 certificate, SubjectIdentifierType type, object o, int version)
	{
		_digest = new Oid(CryptoConfig.MapNameToOID(hashName));
		_certificate = certificate;
		_counter = new SignerInfoCollection();
		_signed = new CryptographicAttributeObjectCollection();
		_unsigned = new CryptographicAttributeObjectCollection();
		_signer = new SubjectIdentifier(type, o);
		_version = version;
	}

	[System.MonoTODO]
	public void CheckHash()
	{
	}

	[System.MonoTODO]
	public void CheckSignature(bool verifySignatureOnly)
	{
	}

	[System.MonoTODO]
	public void CheckSignature(X509Certificate2Collection extraStore, bool verifySignatureOnly)
	{
	}

	[System.MonoTODO]
	public void ComputeCounterSignature()
	{
	}

	[System.MonoTODO]
	public void ComputeCounterSignature(CmsSigner signer)
	{
	}

	[System.MonoTODO]
	public void RemoveCounterSignature(SignerInfo counterSignerInfo)
	{
	}

	[System.MonoTODO]
	public void RemoveCounterSignature(int index)
	{
	}

	internal SignerInfo()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

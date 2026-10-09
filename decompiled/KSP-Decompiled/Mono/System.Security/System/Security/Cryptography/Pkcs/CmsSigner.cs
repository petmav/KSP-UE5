using System.Security.Cryptography.X509Certificates;

namespace System.Security.Cryptography.Pkcs;

public sealed class CmsSigner
{
	private SubjectIdentifierType _signer;

	private X509Certificate2 _certificate;

	private X509Certificate2Collection _coll;

	private Oid _digest;

	private X509IncludeOption _options;

	private CryptographicAttributeObjectCollection _signed;

	private CryptographicAttributeObjectCollection _unsigned;

	public CryptographicAttributeObjectCollection SignedAttributes => _signed;

	public X509Certificate2 Certificate
	{
		get
		{
			return _certificate;
		}
		set
		{
			_certificate = value;
		}
	}

	public X509Certificate2Collection Certificates => _coll;

	public Oid DigestAlgorithm
	{
		get
		{
			return _digest;
		}
		set
		{
			_digest = value;
		}
	}

	public X509IncludeOption IncludeOption
	{
		get
		{
			return _options;
		}
		set
		{
			_options = value;
		}
	}

	public SubjectIdentifierType SignerIdentifierType
	{
		get
		{
			return _signer;
		}
		set
		{
			if (value == SubjectIdentifierType.Unknown)
			{
				throw new ArgumentException("value");
			}
			_signer = value;
		}
	}

	public CryptographicAttributeObjectCollection UnsignedAttributes => _unsigned;

	public CmsSigner()
	{
		_signer = SubjectIdentifierType.IssuerAndSerialNumber;
		_digest = new Oid("1.3.14.3.2.26");
		_options = X509IncludeOption.ExcludeRoot;
		_signed = new CryptographicAttributeObjectCollection();
		_unsigned = new CryptographicAttributeObjectCollection();
		_coll = new X509Certificate2Collection();
	}

	public CmsSigner(SubjectIdentifierType signerIdentifierType)
		: this()
	{
		if (signerIdentifierType == SubjectIdentifierType.Unknown)
		{
			_signer = SubjectIdentifierType.IssuerAndSerialNumber;
		}
		else
		{
			_signer = signerIdentifierType;
		}
	}

	public CmsSigner(SubjectIdentifierType signerIdentifierType, X509Certificate2 certificate)
		: this(signerIdentifierType)
	{
		_certificate = certificate;
	}

	public CmsSigner(X509Certificate2 certificate)
		: this()
	{
		_certificate = certificate;
	}

	[System.MonoTODO]
	public CmsSigner(CspParameters parameters)
		: this()
	{
	}
}

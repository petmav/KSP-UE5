using System.Security.Cryptography.X509Certificates;
using System.Security.Cryptography.Xml;
using System.Text;
using Mono.Security;

namespace System.Security.Cryptography.Pkcs;

public sealed class EnvelopedCms
{
	private ContentInfo _content;

	private AlgorithmIdentifier _identifier;

	private X509Certificate2Collection _certs;

	private RecipientInfoCollection _recipients;

	private CryptographicAttributeObjectCollection _uattribs;

	private SubjectIdentifierType _idType;

	private int _version;

	public X509Certificate2Collection Certificates => _certs;

	public AlgorithmIdentifier ContentEncryptionAlgorithm
	{
		get
		{
			if (_identifier == null)
			{
				_identifier = new AlgorithmIdentifier();
			}
			return _identifier;
		}
	}

	public ContentInfo ContentInfo
	{
		get
		{
			if (_content == null)
			{
				Oid contentType = new Oid("1.2.840.113549.1.7.1");
				_content = new ContentInfo(contentType, new byte[0]);
			}
			return _content;
		}
	}

	public RecipientInfoCollection RecipientInfos => _recipients;

	public CryptographicAttributeObjectCollection UnprotectedAttributes => _uattribs;

	public int Version => _version;

	public EnvelopedCms()
	{
		_certs = new X509Certificate2Collection();
		_recipients = new RecipientInfoCollection();
		_uattribs = new CryptographicAttributeObjectCollection();
	}

	public EnvelopedCms(ContentInfo contentInfo)
		: this()
	{
		if (contentInfo == null)
		{
			throw new ArgumentNullException("contentInfo");
		}
		_content = contentInfo;
	}

	public EnvelopedCms(ContentInfo contentInfo, AlgorithmIdentifier encryptionAlgorithm)
		: this(contentInfo)
	{
		if (encryptionAlgorithm == null)
		{
			throw new ArgumentNullException("encryptionAlgorithm");
		}
		_identifier = encryptionAlgorithm;
	}

	public EnvelopedCms(SubjectIdentifierType recipientIdentifierType, ContentInfo contentInfo)
		: this(contentInfo)
	{
		_idType = recipientIdentifierType;
		if (_idType == SubjectIdentifierType.SubjectKeyIdentifier)
		{
			_version = 2;
		}
	}

	public EnvelopedCms(SubjectIdentifierType recipientIdentifierType, ContentInfo contentInfo, AlgorithmIdentifier encryptionAlgorithm)
		: this(contentInfo, encryptionAlgorithm)
	{
		_idType = recipientIdentifierType;
		if (_idType == SubjectIdentifierType.SubjectKeyIdentifier)
		{
			_version = 2;
		}
	}

	private X509IssuerSerial GetIssuerSerial(string issuer, byte[] serial)
	{
		X509IssuerSerial result = default(X509IssuerSerial);
		result.IssuerName = issuer;
		StringBuilder stringBuilder = new StringBuilder();
		foreach (byte b in serial)
		{
			stringBuilder.Append(b.ToString("X2"));
		}
		result.SerialNumber = stringBuilder.ToString();
		return result;
	}

	[System.MonoTODO]
	public void Decode(byte[] encodedMessage)
	{
		if (encodedMessage == null)
		{
			throw new ArgumentNullException("encodedMessage");
		}
		PKCS7.ContentInfo contentInfo = new PKCS7.ContentInfo(encodedMessage);
		if (contentInfo.ContentType != "1.2.840.113549.1.7.3")
		{
			throw new Exception("");
		}
		PKCS7.EnvelopedData envelopedData = new PKCS7.EnvelopedData(contentInfo.Content);
		Oid contentType = new Oid(envelopedData.ContentInfo.ContentType);
		_content = new ContentInfo(contentType, new byte[0]);
		foreach (PKCS7.RecipientInfo recipientInfo in envelopedData.RecipientInfos)
		{
			AlgorithmIdentifier keyEncryptionAlgorithm = new AlgorithmIdentifier(new Oid(recipientInfo.Oid));
			SubjectIdentifier recipientIdentifier = null;
			if (recipientInfo.SubjectKeyIdentifier != null)
			{
				recipientIdentifier = new SubjectIdentifier(SubjectIdentifierType.SubjectKeyIdentifier, recipientInfo.SubjectKeyIdentifier);
			}
			else if (recipientInfo.Issuer != null && recipientInfo.Serial != null)
			{
				X509IssuerSerial issuerSerial = GetIssuerSerial(recipientInfo.Issuer, recipientInfo.Serial);
				recipientIdentifier = new SubjectIdentifier(SubjectIdentifierType.IssuerAndSerialNumber, issuerSerial);
			}
			KeyTransRecipientInfo ri = new KeyTransRecipientInfo(recipientInfo.Key, keyEncryptionAlgorithm, recipientIdentifier, recipientInfo.Version);
			_recipients.Add(ri);
		}
		_version = envelopedData.Version;
	}

	[System.MonoTODO]
	public void Decrypt()
	{
		throw new InvalidOperationException("not encrypted");
	}

	[System.MonoTODO]
	public void Decrypt(RecipientInfo recipientInfo)
	{
		if (recipientInfo == null)
		{
			throw new ArgumentNullException("recipientInfo");
		}
		Decrypt();
	}

	[System.MonoTODO]
	public void Decrypt(RecipientInfo recipientInfo, X509Certificate2Collection extraStore)
	{
		if (recipientInfo == null)
		{
			throw new ArgumentNullException("recipientInfo");
		}
		if (extraStore == null)
		{
			throw new ArgumentNullException("extraStore");
		}
		Decrypt();
	}

	[System.MonoTODO]
	public void Decrypt(X509Certificate2Collection extraStore)
	{
		if (extraStore == null)
		{
			throw new ArgumentNullException("extraStore");
		}
		Decrypt();
	}

	[System.MonoTODO]
	public byte[] Encode()
	{
		throw new InvalidOperationException("not encrypted");
	}

	[System.MonoTODO]
	public void Encrypt()
	{
		if (_content == null || _content.Content == null || _content.Content.Length == 0)
		{
			throw new CryptographicException("no content to encrypt");
		}
	}

	[System.MonoTODO]
	public void Encrypt(CmsRecipient recipient)
	{
		if (recipient == null)
		{
			throw new ArgumentNullException("recipient");
		}
		Encrypt();
	}

	[System.MonoTODO]
	public void Encrypt(CmsRecipientCollection recipients)
	{
		if (recipients == null)
		{
			throw new ArgumentNullException("recipients");
		}
	}
}

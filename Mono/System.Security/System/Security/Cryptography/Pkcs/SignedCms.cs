using System.Security.Cryptography.X509Certificates;
using System.Security.Cryptography.Xml;
using System.Text;
using Mono.Security;
using Mono.Security.X509;

namespace System.Security.Cryptography.Pkcs;

public sealed class SignedCms
{
	private ContentInfo _content;

	private bool _detached;

	private SignerInfoCollection _info;

	private X509Certificate2Collection _certs;

	private SubjectIdentifierType _type;

	private int _version;

	public X509Certificate2Collection Certificates => _certs;

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

	public bool Detached => _detached;

	public SignerInfoCollection SignerInfos => _info;

	public int Version => _version;

	public SignedCms()
	{
		_certs = new X509Certificate2Collection();
		_info = new SignerInfoCollection();
	}

	public SignedCms(ContentInfo contentInfo)
		: this(contentInfo, detached: false)
	{
	}

	public SignedCms(ContentInfo contentInfo, bool detached)
		: this()
	{
		if (contentInfo == null)
		{
			throw new ArgumentNullException("contentInfo");
		}
		_content = contentInfo;
		_detached = detached;
	}

	public SignedCms(SubjectIdentifierType signerIdentifierType)
		: this()
	{
		_type = signerIdentifierType;
	}

	public SignedCms(SubjectIdentifierType signerIdentifierType, ContentInfo contentInfo)
		: this(contentInfo, detached: false)
	{
		_type = signerIdentifierType;
	}

	public SignedCms(SubjectIdentifierType signerIdentifierType, ContentInfo contentInfo, bool detached)
		: this(contentInfo, detached)
	{
		_type = signerIdentifierType;
	}

	[System.MonoTODO]
	public void CheckSignature(bool verifySignatureOnly)
	{
		SignerInfoEnumerator enumerator = _info.GetEnumerator();
		while (enumerator.MoveNext())
		{
			enumerator.Current.CheckSignature(verifySignatureOnly);
		}
	}

	[System.MonoTODO]
	public void CheckSignature(X509Certificate2Collection extraStore, bool verifySignatureOnly)
	{
		SignerInfoEnumerator enumerator = _info.GetEnumerator();
		while (enumerator.MoveNext())
		{
			enumerator.Current.CheckSignature(extraStore, verifySignatureOnly);
		}
	}

	[System.MonoTODO]
	public void CheckHash()
	{
		throw new InvalidOperationException("");
	}

	[System.MonoTODO]
	public void ComputeSignature()
	{
		throw new CryptographicException("");
	}

	[System.MonoTODO]
	public void ComputeSignature(CmsSigner signer)
	{
		ComputeSignature();
	}

	[System.MonoTODO]
	public void ComputeSignature(CmsSigner signer, bool silent)
	{
		ComputeSignature();
	}

	private string ToString(byte[] array, bool reverse)
	{
		StringBuilder stringBuilder = new StringBuilder();
		if (reverse)
		{
			for (int num = array.Length - 1; num >= 0; num--)
			{
				stringBuilder.Append(array[num].ToString("X2"));
			}
		}
		else
		{
			for (int i = 0; i < array.Length; i++)
			{
				stringBuilder.Append(array[i].ToString("X2"));
			}
		}
		return stringBuilder.ToString();
	}

	private byte[] GetKeyIdentifier(Mono.Security.X509.X509Certificate x509)
	{
		Mono.Security.X509.X509Extension x509Extension = x509.Extensions["2.5.29.14"];
		if (x509Extension != null)
		{
			return new ASN1(x509Extension.Value.Value).Value;
		}
		ASN1 aSN = new ASN1(48);
		ASN1 aSN2 = aSN.Add(new ASN1(48));
		aSN2.Add(new ASN1(CryptoConfig.EncodeOID(x509.KeyAlgorithm)));
		aSN2.Add(new ASN1(x509.KeyAlgorithmParameters));
		byte[] publicKey = x509.PublicKey;
		byte[] array = new byte[publicKey.Length + 1];
		Array.Copy(publicKey, 0, array, 1, publicKey.Length);
		aSN.Add(new ASN1(3, array));
		return SHA1.Create().ComputeHash(aSN.GetBytes());
	}

	[System.MonoTODO("incomplete - missing attributes")]
	public void Decode(byte[] encodedMessage)
	{
		PKCS7.ContentInfo contentInfo = new PKCS7.ContentInfo(encodedMessage);
		if (contentInfo.ContentType != "1.2.840.113549.1.7.2")
		{
			throw new Exception("");
		}
		PKCS7.SignedData signedData = new PKCS7.SignedData(contentInfo.Content);
		SubjectIdentifierType type = SubjectIdentifierType.Unknown;
		object o = null;
		X509Certificate2 certificate = null;
		if (signedData.SignerInfo.Certificate != null)
		{
			certificate = new X509Certificate2(signedData.SignerInfo.Certificate.RawData);
		}
		else if (signedData.SignerInfo.IssuerName != null && signedData.SignerInfo.SerialNumber != null)
		{
			byte[] serialNumber = signedData.SignerInfo.SerialNumber;
			Array.Reverse(serialNumber);
			type = SubjectIdentifierType.IssuerAndSerialNumber;
			X509IssuerSerial x509IssuerSerial = default(X509IssuerSerial);
			x509IssuerSerial.IssuerName = signedData.SignerInfo.IssuerName;
			x509IssuerSerial.SerialNumber = ToString(serialNumber, reverse: true);
			o = x509IssuerSerial;
			foreach (Mono.Security.X509.X509Certificate certificate2 in signedData.Certificates)
			{
				if (certificate2.IssuerName == signedData.SignerInfo.IssuerName && ToString(certificate2.SerialNumber, reverse: true) == x509IssuerSerial.SerialNumber)
				{
					certificate = new X509Certificate2(certificate2.RawData);
					break;
				}
			}
		}
		else if (signedData.SignerInfo.SubjectKeyIdentifier != null)
		{
			string text = ToString(signedData.SignerInfo.SubjectKeyIdentifier, reverse: false);
			type = SubjectIdentifierType.SubjectKeyIdentifier;
			o = text;
			foreach (Mono.Security.X509.X509Certificate certificate3 in signedData.Certificates)
			{
				if (ToString(GetKeyIdentifier(certificate3), reverse: false) == text)
				{
					certificate = new X509Certificate2(certificate3.RawData);
					break;
				}
			}
		}
		SignerInfo signer = new SignerInfo(signedData.SignerInfo.HashName, certificate, type, o, signedData.SignerInfo.Version);
		_info.Add(signer);
		ASN1 content = signedData.ContentInfo.Content;
		Oid contentType = new Oid(signedData.ContentInfo.ContentType);
		if (!_detached || _content == null)
		{
			if (content[0] == null)
			{
				throw new ArgumentException("ContentInfo has no content. Detached signature ?");
			}
			_content = new ContentInfo(contentType, content[0].Value);
		}
		foreach (Mono.Security.X509.X509Certificate certificate4 in signedData.Certificates)
		{
			_certs.Add(new X509Certificate2(certificate4.RawData));
		}
		_version = signedData.Version;
	}

	[System.MonoTODO]
	public byte[] Encode()
	{
		return null;
	}

	[System.MonoTODO]
	public void RemoveSignature(SignerInfo signerInfo)
	{
	}

	[System.MonoTODO]
	public void RemoveSignature(int index)
	{
	}
}

using Mono.Security;

namespace System.Security.Cryptography.Pkcs;

public sealed class Pkcs9ContentType : Pkcs9AttributeObject
{
	internal const string oid = "1.2.840.113549.1.9.3";

	internal const string friendlyName = "Content Type";

	private Oid _contentType;

	private byte[] _encoded;

	public Oid ContentType
	{
		get
		{
			if (_encoded != null)
			{
				Decode(_encoded);
			}
			return _contentType;
		}
	}

	public Pkcs9ContentType()
	{
		((AsnEncodedData)this).Oid = new Oid("1.2.840.113549.1.9.3", "Content Type");
		_encoded = null;
	}

	internal Pkcs9ContentType(string contentType)
	{
		((AsnEncodedData)this).Oid = new Oid("1.2.840.113549.1.9.3", "Content Type");
		_contentType = new Oid(contentType);
		base.RawData = Encode();
		_encoded = null;
	}

	internal Pkcs9ContentType(byte[] encodedContentType)
	{
		if (encodedContentType == null)
		{
			throw new ArgumentNullException("encodedContentType");
		}
		((AsnEncodedData)this).Oid = new Oid("1.2.840.113549.1.9.3", "Content Type");
		base.RawData = encodedContentType;
		Decode(encodedContentType);
	}

	public override void CopyFrom(AsnEncodedData asnEncodedData)
	{
		base.CopyFrom(asnEncodedData);
		_encoded = asnEncodedData.RawData;
	}

	internal void Decode(byte[] attribute)
	{
		if (attribute == null || attribute[0] != 6)
		{
			throw new CryptographicException(global::Locale.GetText("Expected an OID."));
		}
		ASN1 asn = new ASN1(attribute);
		_contentType = new Oid(ASN1Convert.ToOid(asn));
		_encoded = null;
	}

	internal byte[] Encode()
	{
		if (_contentType == null)
		{
			return null;
		}
		return ASN1Convert.FromOid(_contentType.Value).GetBytes();
	}
}

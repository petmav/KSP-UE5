using Mono.Security;

namespace System.Security.Cryptography.Pkcs;

public sealed class Pkcs9MessageDigest : Pkcs9AttributeObject
{
	internal const string oid = "1.2.840.113549.1.9.4";

	internal const string friendlyName = "Message Digest";

	private byte[] _messageDigest;

	private byte[] _encoded;

	public byte[] MessageDigest
	{
		get
		{
			if (_encoded != null)
			{
				Decode(_encoded);
			}
			return _messageDigest;
		}
	}

	public Pkcs9MessageDigest()
	{
		((AsnEncodedData)this).Oid = new Oid("1.2.840.113549.1.9.4", "Message Digest");
		_encoded = null;
	}

	internal Pkcs9MessageDigest(byte[] messageDigest, bool encoded)
	{
		if (messageDigest == null)
		{
			throw new ArgumentNullException("messageDigest");
		}
		if (encoded)
		{
			((AsnEncodedData)this).Oid = new Oid("1.2.840.113549.1.9.4", "Message Digest");
			base.RawData = messageDigest;
			Decode(messageDigest);
		}
		else
		{
			((AsnEncodedData)this).Oid = new Oid("1.2.840.113549.1.9.4", "Message Digest");
			_messageDigest = (byte[])_messageDigest.Clone();
			base.RawData = Encode();
		}
	}

	public override void CopyFrom(AsnEncodedData asnEncodedData)
	{
		base.CopyFrom(asnEncodedData);
		_encoded = asnEncodedData.RawData;
	}

	internal void Decode(byte[] attribute)
	{
		if (attribute == null || attribute[0] != 4)
		{
			throw new CryptographicException(global::Locale.GetText("Expected an OCTETSTRING."));
		}
		ASN1 aSN = new ASN1(attribute);
		_messageDigest = aSN.Value;
		_encoded = null;
	}

	internal byte[] Encode()
	{
		return new ASN1(4, _messageDigest).GetBytes();
	}
}

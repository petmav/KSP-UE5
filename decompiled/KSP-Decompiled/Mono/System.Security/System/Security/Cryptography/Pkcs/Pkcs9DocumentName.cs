using System.Text;
using Mono.Security;

namespace System.Security.Cryptography.Pkcs;

public sealed class Pkcs9DocumentName : Pkcs9AttributeObject
{
	internal const string oid = "1.3.6.1.4.1.311.88.2.1";

	internal const string friendlyName = null;

	private string _name;

	public string DocumentName => _name;

	public Pkcs9DocumentName()
	{
		((AsnEncodedData)this).Oid = new Oid("1.3.6.1.4.1.311.88.2.1", null);
	}

	public Pkcs9DocumentName(string documentName)
	{
		if (documentName == null)
		{
			throw new ArgumentNullException("documentName");
		}
		((AsnEncodedData)this).Oid = new Oid("1.3.6.1.4.1.311.88.2.1", null);
		_name = documentName;
		base.RawData = Encode();
	}

	public Pkcs9DocumentName(byte[] encodedDocumentName)
	{
		if (encodedDocumentName == null)
		{
			throw new ArgumentNullException("encodedDocumentName");
		}
		((AsnEncodedData)this).Oid = new Oid("1.3.6.1.4.1.311.88.2.1", null);
		base.RawData = encodedDocumentName;
		Decode(encodedDocumentName);
	}

	public override void CopyFrom(AsnEncodedData asnEncodedData)
	{
		base.CopyFrom(asnEncodedData);
		Decode(base.RawData);
	}

	internal void Decode(byte[] attribute)
	{
		if (attribute[0] == 4)
		{
			byte[] value = new ASN1(attribute).Value;
			int num = value.Length;
			if (value[num - 2] == 0)
			{
				num -= 2;
			}
			_name = Encoding.Unicode.GetString(value, 0, num);
		}
	}

	internal byte[] Encode()
	{
		return new ASN1(4, Encoding.Unicode.GetBytes(_name + "\0")).GetBytes();
	}
}

using System.Globalization;
using System.Text;
using Mono.Security;

namespace System.Security.Cryptography.Pkcs;

public sealed class Pkcs9SigningTime : Pkcs9AttributeObject
{
	internal const string oid = "1.2.840.113549.1.9.5";

	internal const string friendlyName = "Signing Time";

	private DateTime _signingTime;

	public DateTime SigningTime => _signingTime;

	public Pkcs9SigningTime()
	{
		((AsnEncodedData)this).Oid = new Oid("1.2.840.113549.1.9.5", "Signing Time");
		_signingTime = DateTime.Now;
		base.RawData = Encode();
	}

	public Pkcs9SigningTime(DateTime signingTime)
	{
		((AsnEncodedData)this).Oid = new Oid("1.2.840.113549.1.9.5", "Signing Time");
		_signingTime = signingTime;
		base.RawData = Encode();
	}

	public Pkcs9SigningTime(byte[] encodedSigningTime)
	{
		if (encodedSigningTime == null)
		{
			throw new ArgumentNullException("encodedSigningTime");
		}
		((AsnEncodedData)this).Oid = new Oid("1.2.840.113549.1.9.5", "Signing Time");
		base.RawData = encodedSigningTime;
		Decode(encodedSigningTime);
	}

	public override void CopyFrom(AsnEncodedData asnEncodedData)
	{
		if (asnEncodedData == null)
		{
			throw new ArgumentNullException("asnEncodedData");
		}
		Decode(asnEncodedData.RawData);
		base.Oid = asnEncodedData.Oid;
		base.RawData = asnEncodedData.RawData;
	}

	internal void Decode(byte[] attribute)
	{
		if (attribute[0] != 23)
		{
			throw new CryptographicException(global::Locale.GetText("Only UTCTIME is supported."));
		}
		byte[] value = new ASN1(attribute).Value;
		string @string = Encoding.ASCII.GetString(value, 0, value.Length - 1);
		_signingTime = DateTime.ParseExact(@string, "yyMMddHHmmss", null);
	}

	internal byte[] Encode()
	{
		if (_signingTime.Year <= 1600)
		{
			throw new ArgumentOutOfRangeException("<= 1600");
		}
		if (_signingTime.Year < 1950 || _signingTime.Year >= 2050)
		{
			throw new CryptographicException("[1950,2049]");
		}
		string s = _signingTime.ToString("yyMMddHHmmss", CultureInfo.InvariantCulture) + "Z";
		return new ASN1(23, Encoding.ASCII.GetBytes(s)).GetBytes();
	}
}

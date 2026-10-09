namespace System.Security.Cryptography.Xml;

public struct X509IssuerSerial
{
	private string _issuerName;

	private string _serialNumber;

	public string IssuerName
	{
		get
		{
			return _issuerName;
		}
		set
		{
			_issuerName = value;
		}
	}

	public string SerialNumber
	{
		get
		{
			return _serialNumber;
		}
		set
		{
			_serialNumber = value;
		}
	}

	internal X509IssuerSerial(string issuer, string serial)
	{
		_issuerName = issuer;
		_serialNumber = serial;
	}
}

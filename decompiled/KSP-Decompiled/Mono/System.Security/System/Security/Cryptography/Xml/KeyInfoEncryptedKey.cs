using System.Xml;

namespace System.Security.Cryptography.Xml;

public class KeyInfoEncryptedKey : KeyInfoClause
{
	private EncryptedKey _encryptedKey;

	public EncryptedKey EncryptedKey
	{
		get
		{
			return _encryptedKey;
		}
		set
		{
			_encryptedKey = value;
		}
	}

	public KeyInfoEncryptedKey()
	{
	}

	public KeyInfoEncryptedKey(EncryptedKey encryptedKey)
	{
		_encryptedKey = encryptedKey;
	}

	public override XmlElement GetXml()
	{
		if (_encryptedKey == null)
		{
			throw new CryptographicException("Malformed element {0}.", "KeyInfoEncryptedKey");
		}
		return _encryptedKey.GetXml();
	}

	internal override XmlElement GetXml(XmlDocument xmlDocument)
	{
		if (_encryptedKey == null)
		{
			throw new CryptographicException("Malformed element {0}.", "KeyInfoEncryptedKey");
		}
		return _encryptedKey.GetXml(xmlDocument);
	}

	public override void LoadXml(XmlElement value)
	{
		_encryptedKey = new EncryptedKey();
		_encryptedKey.LoadXml(value);
	}
}

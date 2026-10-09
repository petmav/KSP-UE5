using System.Xml;

namespace System.Security.Cryptography.Xml;

public class KeyInfoName : KeyInfoClause
{
	private string _keyName;

	public string Value
	{
		get
		{
			return _keyName;
		}
		set
		{
			_keyName = value;
		}
	}

	public KeyInfoName()
		: this(null)
	{
	}

	public KeyInfoName(string keyName)
	{
		Value = keyName;
	}

	public override XmlElement GetXml()
	{
		XmlDocument xmlDocument = new XmlDocument();
		xmlDocument.PreserveWhitespace = true;
		return GetXml(xmlDocument);
	}

	internal override XmlElement GetXml(XmlDocument xmlDocument)
	{
		XmlElement xmlElement = xmlDocument.CreateElement("KeyName", "http://www.w3.org/2000/09/xmldsig#");
		xmlElement.AppendChild(xmlDocument.CreateTextNode(_keyName));
		return xmlElement;
	}

	public override void LoadXml(XmlElement value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		_keyName = value.InnerText.Trim();
	}
}

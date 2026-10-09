using System.Xml;

namespace System.Security.Cryptography.Xml;

public class EncryptionMethod
{
	private XmlElement _cachedXml;

	private int _keySize;

	private string _algorithm;

	private bool CacheValid => _cachedXml != null;

	public int KeySize
	{
		get
		{
			return _keySize;
		}
		set
		{
			if (value <= 0)
			{
				throw new ArgumentOutOfRangeException("value", "The key size should be a non negative integer.");
			}
			_keySize = value;
			_cachedXml = null;
		}
	}

	public string KeyAlgorithm
	{
		get
		{
			return _algorithm;
		}
		set
		{
			_algorithm = value;
			_cachedXml = null;
		}
	}

	public EncryptionMethod()
	{
		_cachedXml = null;
	}

	public EncryptionMethod(string algorithm)
	{
		_algorithm = algorithm;
		_cachedXml = null;
	}

	public XmlElement GetXml()
	{
		if (CacheValid)
		{
			return _cachedXml;
		}
		XmlDocument xmlDocument = new XmlDocument();
		xmlDocument.PreserveWhitespace = true;
		return GetXml(xmlDocument);
	}

	internal XmlElement GetXml(XmlDocument document)
	{
		XmlElement xmlElement = document.CreateElement("EncryptionMethod", "http://www.w3.org/2001/04/xmlenc#");
		if (!string.IsNullOrEmpty(_algorithm))
		{
			xmlElement.SetAttribute("Algorithm", _algorithm);
		}
		if (_keySize > 0)
		{
			XmlElement xmlElement2 = document.CreateElement("KeySize", "http://www.w3.org/2001/04/xmlenc#");
			xmlElement2.AppendChild(document.CreateTextNode(_keySize.ToString(null, null)));
			xmlElement.AppendChild(xmlElement2);
		}
		return xmlElement;
	}

	public void LoadXml(XmlElement value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		XmlNamespaceManager xmlNamespaceManager = new XmlNamespaceManager(value.OwnerDocument.NameTable);
		xmlNamespaceManager.AddNamespace("enc", "http://www.w3.org/2001/04/xmlenc#");
		_algorithm = Utils.GetAttribute(value, "Algorithm", "http://www.w3.org/2001/04/xmlenc#");
		XmlNode xmlNode = value.SelectSingleNode("enc:KeySize", xmlNamespaceManager);
		if (xmlNode != null)
		{
			KeySize = Convert.ToInt32(Utils.DiscardWhiteSpaces(xmlNode.InnerText), null);
		}
		_cachedXml = value;
	}
}

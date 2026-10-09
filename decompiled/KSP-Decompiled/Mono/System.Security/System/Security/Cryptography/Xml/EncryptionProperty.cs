using System.Xml;

namespace System.Security.Cryptography.Xml;

public sealed class EncryptionProperty
{
	private string _target;

	private string _id;

	private XmlElement _elemProp;

	private XmlElement _cachedXml;

	public string Id => _id;

	public string Target => _target;

	public XmlElement PropertyElement
	{
		get
		{
			return _elemProp;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			if (value.LocalName != "EncryptionProperty" || value.NamespaceURI != "http://www.w3.org/2001/04/xmlenc#")
			{
				throw new CryptographicException("Malformed encryption property element.");
			}
			_elemProp = value;
			_cachedXml = null;
		}
	}

	private bool CacheValid => _cachedXml != null;

	public EncryptionProperty()
	{
	}

	public EncryptionProperty(XmlElement elementProperty)
	{
		if (elementProperty == null)
		{
			throw new ArgumentNullException("elementProperty");
		}
		if (elementProperty.LocalName != "EncryptionProperty" || elementProperty.NamespaceURI != "http://www.w3.org/2001/04/xmlenc#")
		{
			throw new CryptographicException("Malformed encryption property element.");
		}
		_elemProp = elementProperty;
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
		return document.ImportNode(_elemProp, deep: true) as XmlElement;
	}

	public void LoadXml(XmlElement value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (value.LocalName != "EncryptionProperty" || value.NamespaceURI != "http://www.w3.org/2001/04/xmlenc#")
		{
			throw new CryptographicException("Malformed encryption property element.");
		}
		_cachedXml = value;
		_id = Utils.GetAttribute(value, "Id", "http://www.w3.org/2001/04/xmlenc#");
		_target = Utils.GetAttribute(value, "Target", "http://www.w3.org/2001/04/xmlenc#");
		_elemProp = value;
	}
}

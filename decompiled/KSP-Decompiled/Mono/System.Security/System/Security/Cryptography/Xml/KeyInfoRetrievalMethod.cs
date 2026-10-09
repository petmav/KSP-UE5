using System.Xml;

namespace System.Security.Cryptography.Xml;

public class KeyInfoRetrievalMethod : KeyInfoClause
{
	private string _uri;

	private string _type;

	public string Uri
	{
		get
		{
			return _uri;
		}
		set
		{
			_uri = value;
		}
	}

	public string Type
	{
		get
		{
			return _type;
		}
		set
		{
			_type = value;
		}
	}

	public KeyInfoRetrievalMethod()
	{
	}

	public KeyInfoRetrievalMethod(string strUri)
	{
		_uri = strUri;
	}

	public KeyInfoRetrievalMethod(string strUri, string typeName)
	{
		_uri = strUri;
		_type = typeName;
	}

	public override XmlElement GetXml()
	{
		XmlDocument xmlDocument = new XmlDocument();
		xmlDocument.PreserveWhitespace = true;
		return GetXml(xmlDocument);
	}

	internal override XmlElement GetXml(XmlDocument xmlDocument)
	{
		XmlElement xmlElement = xmlDocument.CreateElement("RetrievalMethod", "http://www.w3.org/2000/09/xmldsig#");
		if (!string.IsNullOrEmpty(_uri))
		{
			xmlElement.SetAttribute("URI", _uri);
		}
		if (!string.IsNullOrEmpty(_type))
		{
			xmlElement.SetAttribute("Type", _type);
		}
		return xmlElement;
	}

	public override void LoadXml(XmlElement value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		_uri = Utils.GetAttribute(value, "URI", "http://www.w3.org/2000/09/xmldsig#");
		_type = Utils.GetAttribute(value, "Type", "http://www.w3.org/2000/09/xmldsig#");
	}
}

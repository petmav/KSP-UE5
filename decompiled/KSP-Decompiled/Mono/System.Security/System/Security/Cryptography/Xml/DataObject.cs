using System.Xml;

namespace System.Security.Cryptography.Xml;

public class DataObject
{
	private string _id;

	private string _mimeType;

	private string _encoding;

	private CanonicalXmlNodeList _elData;

	private XmlElement _cachedXml;

	public string Id
	{
		get
		{
			return _id;
		}
		set
		{
			_id = value;
			_cachedXml = null;
		}
	}

	public string MimeType
	{
		get
		{
			return _mimeType;
		}
		set
		{
			_mimeType = value;
			_cachedXml = null;
		}
	}

	public string Encoding
	{
		get
		{
			return _encoding;
		}
		set
		{
			_encoding = value;
			_cachedXml = null;
		}
	}

	public XmlNodeList Data
	{
		get
		{
			return _elData;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			_elData = new CanonicalXmlNodeList();
			foreach (XmlNode item in value)
			{
				_elData.Add(item);
			}
			_cachedXml = null;
		}
	}

	private bool CacheValid => _cachedXml != null;

	public DataObject()
	{
		_cachedXml = null;
		_elData = new CanonicalXmlNodeList();
	}

	public DataObject(string id, string mimeType, string encoding, XmlElement data)
	{
		if (data == null)
		{
			throw new ArgumentNullException("data");
		}
		_id = id;
		_mimeType = mimeType;
		_encoding = encoding;
		_elData = new CanonicalXmlNodeList();
		_elData.Add(data);
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
		XmlElement xmlElement = document.CreateElement("Object", "http://www.w3.org/2000/09/xmldsig#");
		if (!string.IsNullOrEmpty(_id))
		{
			xmlElement.SetAttribute("Id", _id);
		}
		if (!string.IsNullOrEmpty(_mimeType))
		{
			xmlElement.SetAttribute("MimeType", _mimeType);
		}
		if (!string.IsNullOrEmpty(_encoding))
		{
			xmlElement.SetAttribute("Encoding", _encoding);
		}
		if (_elData != null)
		{
			foreach (XmlNode elDatum in _elData)
			{
				xmlElement.AppendChild(document.ImportNode(elDatum, deep: true));
			}
		}
		return xmlElement;
	}

	public void LoadXml(XmlElement value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		_id = Utils.GetAttribute(value, "Id", "http://www.w3.org/2000/09/xmldsig#");
		_mimeType = Utils.GetAttribute(value, "MimeType", "http://www.w3.org/2000/09/xmldsig#");
		_encoding = Utils.GetAttribute(value, "Encoding", "http://www.w3.org/2000/09/xmldsig#");
		foreach (XmlNode childNode in value.ChildNodes)
		{
			_elData.Add(childNode);
		}
		_cachedXml = value;
	}
}

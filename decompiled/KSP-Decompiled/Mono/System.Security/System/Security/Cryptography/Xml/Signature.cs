using System.Collections;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public class Signature
{
	private static XmlNamespaceManager dsigNsmgr;

	private ArrayList list;

	private SignedInfo info;

	private KeyInfo key;

	private string id;

	private byte[] signature;

	private XmlElement element;

	public string Id
	{
		get
		{
			return id;
		}
		set
		{
			element = null;
			id = value;
		}
	}

	public KeyInfo KeyInfo
	{
		get
		{
			return key;
		}
		set
		{
			element = null;
			key = value;
		}
	}

	public IList ObjectList
	{
		get
		{
			return list;
		}
		set
		{
			list = ArrayList.Adapter(value);
		}
	}

	public byte[] SignatureValue
	{
		get
		{
			return signature;
		}
		set
		{
			element = null;
			signature = value;
		}
	}

	public SignedInfo SignedInfo
	{
		get
		{
			return info;
		}
		set
		{
			element = null;
			info = value;
		}
	}

	static Signature()
	{
		dsigNsmgr = new XmlNamespaceManager(new NameTable());
		dsigNsmgr.AddNamespace("xd", "http://www.w3.org/2000/09/xmldsig#");
	}

	public Signature()
	{
		list = new ArrayList();
	}

	public void AddObject(DataObject dataObject)
	{
		list.Add(dataObject);
	}

	public XmlElement GetXml()
	{
		return GetXml(null);
	}

	internal XmlElement GetXml(XmlDocument document)
	{
		if (element != null)
		{
			return element;
		}
		if (info == null)
		{
			throw new CryptographicException("SignedInfo");
		}
		if (signature == null)
		{
			throw new CryptographicException("SignatureValue");
		}
		if (document == null)
		{
			document = new XmlDocument();
		}
		XmlElement xmlElement = document.CreateElement("Signature", "http://www.w3.org/2000/09/xmldsig#");
		if (id != null)
		{
			xmlElement.SetAttribute("Id", id);
		}
		XmlNode xml = info.GetXml();
		XmlNode newChild = document.ImportNode(xml, deep: true);
		xmlElement.AppendChild(newChild);
		if (signature != null)
		{
			XmlElement xmlElement2 = document.CreateElement("SignatureValue", "http://www.w3.org/2000/09/xmldsig#");
			xmlElement2.InnerText = Convert.ToBase64String(signature);
			xmlElement.AppendChild(xmlElement2);
		}
		if (key != null)
		{
			xml = key.GetXml();
			newChild = document.ImportNode(xml, deep: true);
			xmlElement.AppendChild(newChild);
		}
		if (list.Count > 0)
		{
			foreach (DataObject item in list)
			{
				xml = item.GetXml();
				newChild = document.ImportNode(xml, deep: true);
				xmlElement.AppendChild(newChild);
			}
		}
		return xmlElement;
	}

	private string GetAttribute(XmlElement xel, string attribute)
	{
		return xel.Attributes[attribute]?.InnerText;
	}

	public void LoadXml(XmlElement value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (value.LocalName == "Signature" && value.NamespaceURI == "http://www.w3.org/2000/09/xmldsig#")
		{
			id = GetAttribute(value, "Id");
			int num = NextElementPos(value.ChildNodes, 0, "SignedInfo", "http://www.w3.org/2000/09/xmldsig#", required: true);
			XmlElement value2 = (XmlElement)value.ChildNodes[num];
			info = new SignedInfo();
			info.LoadXml(value2);
			num = NextElementPos(value.ChildNodes, ++num, "SignatureValue", "http://www.w3.org/2000/09/xmldsig#", required: true);
			XmlElement xmlElement = (XmlElement)value.ChildNodes[num];
			signature = Convert.FromBase64String(xmlElement.InnerText);
			num = NextElementPos(value.ChildNodes, ++num, "KeyInfo", "http://www.w3.org/2000/09/xmldsig#", required: false);
			if (num > 0)
			{
				XmlElement value3 = (XmlElement)value.ChildNodes[num];
				key = new KeyInfo();
				key.LoadXml(value3);
			}
			foreach (XmlElement item in value.SelectNodes("xd:Object", dsigNsmgr))
			{
				DataObject dataObject = new DataObject();
				dataObject.LoadXml(item);
				AddObject(dataObject);
			}
			if (info == null)
			{
				throw new CryptographicException("SignedInfo");
			}
			if (signature == null)
			{
				throw new CryptographicException("SignatureValue");
			}
			return;
		}
		throw new CryptographicException("Malformed element: Signature.");
	}

	private int NextElementPos(XmlNodeList nl, int pos, string name, string ns, bool required)
	{
		while (pos < nl.Count)
		{
			if (nl[pos].NodeType == XmlNodeType.Element)
			{
				if (nl[pos].LocalName != name || nl[pos].NamespaceURI != ns)
				{
					if (required)
					{
						throw new CryptographicException("Malformed element " + name);
					}
					return -2;
				}
				return pos;
			}
			pos++;
		}
		if (required)
		{
			throw new CryptographicException("Malformed element " + name);
		}
		return -1;
	}
}

using System.Collections;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal class Manifest
{
	private ArrayList references;

	private string id;

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

	public ArrayList References => references;

	public Manifest()
	{
		references = new ArrayList();
	}

	public Manifest(XmlElement xel)
		: this()
	{
		LoadXml(xel);
	}

	public void AddReference(Reference reference)
	{
		references.Add(reference);
	}

	public XmlElement GetXml()
	{
		if (element != null)
		{
			return element;
		}
		XmlDocument xmlDocument = new XmlDocument();
		XmlElement xmlElement = xmlDocument.CreateElement("SignedInfo", "http://www.w3.org/2000/09/xmldsig#");
		if (id != null)
		{
			xmlElement.SetAttribute("Id", id);
		}
		foreach (Reference reference in references)
		{
			XmlNode xml = reference.GetXml();
			XmlNode newChild = xmlDocument.ImportNode(xml, deep: true);
			xmlElement.AppendChild(newChild);
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
		if (value.LocalName != "Manifest" || value.NamespaceURI != "http://www.w3.org/2000/09/xmldsig#")
		{
			throw new CryptographicException();
		}
		id = GetAttribute(value, "Id");
		for (int i = 0; i < value.ChildNodes.Count; i++)
		{
			XmlNode xmlNode = value.ChildNodes[i];
			if (xmlNode.NodeType == XmlNodeType.Element && xmlNode.LocalName == "Reference" && xmlNode.NamespaceURI == "http://www.w3.org/2000/09/xmldsig#")
			{
				Reference reference = new Reference();
				reference.LoadXml((XmlElement)xmlNode);
				AddReference(reference);
			}
		}
		element = value;
	}
}

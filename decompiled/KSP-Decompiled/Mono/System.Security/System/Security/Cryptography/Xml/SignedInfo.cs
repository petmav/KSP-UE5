using System.Collections;
using System.Runtime.InteropServices;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public class SignedInfo : ICollection, IEnumerable
{
	private ArrayList references;

	private string c14nMethod;

	private string id;

	private string signatureMethod;

	private string signatureLength;

	private XmlElement element;

	public string CanonicalizationMethod
	{
		get
		{
			return c14nMethod;
		}
		set
		{
			c14nMethod = value;
			element = null;
		}
	}

	[System.MonoTODO]
	[ComVisible(false)]
	public Transform CanonicalizationMethodObject
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public int Count
	{
		get
		{
			throw new NotSupportedException();
		}
	}

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

	public bool IsReadOnly
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public bool IsSynchronized
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public ArrayList References => references;

	public string SignatureLength
	{
		get
		{
			return signatureLength;
		}
		set
		{
			element = null;
			signatureLength = value;
		}
	}

	public string SignatureMethod
	{
		get
		{
			return signatureMethod;
		}
		set
		{
			element = null;
			signatureMethod = value;
		}
	}

	public object SyncRoot
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public SignedInfo()
	{
		references = new ArrayList();
		c14nMethod = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";
	}

	public void AddReference(Reference reference)
	{
		references.Add(reference);
	}

	public void CopyTo(Array array, int index)
	{
		throw new NotSupportedException();
	}

	public IEnumerator GetEnumerator()
	{
		return references.GetEnumerator();
	}

	public XmlElement GetXml()
	{
		if (element != null)
		{
			return element;
		}
		if (signatureMethod == null)
		{
			throw new CryptographicException("SignatureMethod");
		}
		if (references.Count == 0)
		{
			throw new CryptographicException("References empty");
		}
		XmlDocument xmlDocument = new XmlDocument();
		XmlElement xmlElement = xmlDocument.CreateElement("SignedInfo", "http://www.w3.org/2000/09/xmldsig#");
		if (id != null)
		{
			xmlElement.SetAttribute("Id", id);
		}
		if (c14nMethod != null)
		{
			XmlElement xmlElement2 = xmlDocument.CreateElement("CanonicalizationMethod", "http://www.w3.org/2000/09/xmldsig#");
			xmlElement2.SetAttribute("Algorithm", c14nMethod);
			xmlElement.AppendChild(xmlElement2);
		}
		if (signatureMethod != null)
		{
			XmlElement xmlElement3 = xmlDocument.CreateElement("SignatureMethod", "http://www.w3.org/2000/09/xmldsig#");
			xmlElement3.SetAttribute("Algorithm", signatureMethod);
			if (signatureLength != null)
			{
				XmlElement xmlElement4 = xmlDocument.CreateElement("HMACOutputLength", "http://www.w3.org/2000/09/xmldsig#");
				xmlElement4.InnerText = signatureLength;
				xmlElement3.AppendChild(xmlElement4);
			}
			xmlElement.AppendChild(xmlElement3);
		}
		if (references.Count == 0)
		{
			throw new CryptographicException("At least one Reference element is required in SignedInfo.");
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
		if (value.LocalName != "SignedInfo" || value.NamespaceURI != "http://www.w3.org/2000/09/xmldsig#")
		{
			throw new CryptographicException();
		}
		id = GetAttribute(value, "Id");
		c14nMethod = XmlSignature.GetAttributeFromElement(value, "Algorithm", "CanonicalizationMethod");
		XmlElement childElement = XmlSignature.GetChildElement(value, "SignatureMethod", "http://www.w3.org/2000/09/xmldsig#");
		if (childElement != null)
		{
			signatureMethod = childElement.GetAttribute("Algorithm");
			XmlElement childElement2 = XmlSignature.GetChildElement(childElement, "HMACOutputLength", "http://www.w3.org/2000/09/xmldsig#");
			if (childElement2 != null)
			{
				signatureLength = childElement2.InnerText;
			}
		}
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

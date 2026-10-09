using System.IO;
using System.Xml;
using System.Xml.XPath;

namespace System.Security.Cryptography.Xml;

public class XmlDsigXPathTransform : Transform
{
	private Type[] _inputTypes = new Type[3]
	{
		typeof(Stream),
		typeof(XmlNodeList),
		typeof(XmlDocument)
	};

	private Type[] _outputTypes = new Type[1] { typeof(XmlNodeList) };

	private string _xpathexpr;

	private XmlDocument _document;

	private XmlNamespaceManager _nsm;

	public override Type[] InputTypes => _inputTypes;

	public override Type[] OutputTypes => _outputTypes;

	public XmlDsigXPathTransform()
	{
		base.Algorithm = "http://www.w3.org/TR/1999/REC-xpath-19991116";
	}

	public override void LoadInnerXml(XmlNodeList nodeList)
	{
		if (nodeList == null)
		{
			throw new CryptographicException("Unknown transform has been encountered.");
		}
		foreach (XmlNode node in nodeList)
		{
			string text = null;
			string text2 = null;
			if (!(node is XmlElement { LocalName: "XPath" } xmlElement))
			{
				continue;
			}
			_xpathexpr = xmlElement.InnerXml.Trim(null);
			XmlNameTable nameTable = new XmlNodeReader(xmlElement).NameTable;
			_nsm = new XmlNamespaceManager(nameTable);
			foreach (XmlAttribute attribute in xmlElement.Attributes)
			{
				if (attribute.Prefix == "xmlns")
				{
					text = attribute.LocalName;
					text2 = attribute.Value;
					if (text == null)
					{
						text = xmlElement.Prefix;
						text2 = xmlElement.NamespaceURI;
					}
					_nsm.AddNamespace(text, text2);
				}
			}
			break;
		}
		if (_xpathexpr == null)
		{
			throw new CryptographicException("Unknown transform has been encountered.");
		}
	}

	protected override XmlNodeList GetInnerXml()
	{
		XmlDocument xmlDocument = new XmlDocument();
		XmlElement xmlElement = xmlDocument.CreateElement(null, "XPath", "http://www.w3.org/2000/09/xmldsig#");
		if (_nsm != null)
		{
			foreach (string item in _nsm)
			{
				switch (item)
				{
				case "xml":
				case "xmlns":
				case null:
					continue;
				}
				if (item.Length > 0)
				{
					xmlElement.SetAttribute("xmlns:" + item, _nsm.LookupNamespace(item));
				}
			}
		}
		xmlElement.InnerXml = _xpathexpr;
		xmlDocument.AppendChild(xmlElement);
		return xmlDocument.ChildNodes;
	}

	public override void LoadInput(object obj)
	{
		if (obj is Stream)
		{
			LoadStreamInput((Stream)obj);
		}
		else if (obj is XmlNodeList)
		{
			LoadXmlNodeListInput((XmlNodeList)obj);
		}
		else if (obj is XmlDocument)
		{
			LoadXmlDocumentInput((XmlDocument)obj);
		}
	}

	private void LoadStreamInput(Stream stream)
	{
		XmlResolver xmlResolver = (base.ResolverSet ? _xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), base.BaseURI));
		XmlReader reader = Utils.PreProcessStreamInput(stream, xmlResolver, base.BaseURI);
		_document = new XmlDocument();
		_document.PreserveWhitespace = true;
		_document.Load(reader);
	}

	private void LoadXmlNodeListInput(XmlNodeList nodeList)
	{
		XmlResolver resolver = (base.ResolverSet ? _xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), base.BaseURI));
		using MemoryStream stream = new MemoryStream(new CanonicalXml(nodeList, resolver, includeComments: true).GetBytes());
		LoadStreamInput(stream);
	}

	private void LoadXmlDocumentInput(XmlDocument doc)
	{
		_document = doc;
	}

	public override object GetOutput()
	{
		CanonicalXmlNodeList canonicalXmlNodeList = new CanonicalXmlNodeList();
		if (!string.IsNullOrEmpty(_xpathexpr))
		{
			XPathNavigator xPathNavigator = _document.CreateNavigator();
			XPathNodeIterator xPathNodeIterator = xPathNavigator.Select("//. | //@*");
			XPathExpression xPathExpression = xPathNavigator.Compile("boolean(" + _xpathexpr + ")");
			xPathExpression.SetContext(_nsm);
			while (xPathNodeIterator.MoveNext())
			{
				XmlNode node = ((IHasXmlNode)xPathNodeIterator.Current).GetNode();
				if ((bool)xPathNodeIterator.Current.Evaluate(xPathExpression))
				{
					canonicalXmlNodeList.Add(node);
				}
			}
			xPathNodeIterator = xPathNavigator.Select("//namespace::*");
			while (xPathNodeIterator.MoveNext())
			{
				XmlNode node2 = ((IHasXmlNode)xPathNodeIterator.Current).GetNode();
				canonicalXmlNodeList.Add(node2);
			}
		}
		return canonicalXmlNodeList;
	}

	public override object GetOutput(Type type)
	{
		if (type != typeof(XmlNodeList) && !type.IsSubclassOf(typeof(XmlNodeList)))
		{
			throw new ArgumentException("The input type was invalid for this transform.", "type");
		}
		return (XmlNodeList)GetOutput();
	}
}

using System.IO;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public class XmlDsigExcC14NTransform : Transform
{
	private Type[] _inputTypes = new Type[3]
	{
		typeof(Stream),
		typeof(XmlDocument),
		typeof(XmlNodeList)
	};

	private Type[] _outputTypes = new Type[1] { typeof(Stream) };

	private bool _includeComments;

	private string _inclusiveNamespacesPrefixList;

	private ExcCanonicalXml _excCanonicalXml;

	public string InclusiveNamespacesPrefixList
	{
		get
		{
			return _inclusiveNamespacesPrefixList;
		}
		set
		{
			_inclusiveNamespacesPrefixList = value;
		}
	}

	public override Type[] InputTypes => _inputTypes;

	public override Type[] OutputTypes => _outputTypes;

	public XmlDsigExcC14NTransform()
		: this(includeComments: false, null)
	{
	}

	public XmlDsigExcC14NTransform(bool includeComments)
		: this(includeComments, null)
	{
	}

	public XmlDsigExcC14NTransform(string inclusiveNamespacesPrefixList)
		: this(includeComments: false, inclusiveNamespacesPrefixList)
	{
	}

	public XmlDsigExcC14NTransform(bool includeComments, string inclusiveNamespacesPrefixList)
	{
		_includeComments = includeComments;
		_inclusiveNamespacesPrefixList = inclusiveNamespacesPrefixList;
		base.Algorithm = (includeComments ? "http://www.w3.org/2001/10/xml-exc-c14n#WithComments" : "http://www.w3.org/2001/10/xml-exc-c14n#");
	}

	public override void LoadInnerXml(XmlNodeList nodeList)
	{
		if (nodeList == null)
		{
			return;
		}
		foreach (XmlNode node in nodeList)
		{
			if (node is XmlElement xmlElement && xmlElement.LocalName.Equals("InclusiveNamespaces") && xmlElement.NamespaceURI.Equals("http://www.w3.org/2001/10/xml-exc-c14n#") && Utils.HasAttribute(xmlElement, "PrefixList", "http://www.w3.org/2000/09/xmldsig#"))
			{
				InclusiveNamespacesPrefixList = Utils.GetAttribute(xmlElement, "PrefixList", "http://www.w3.org/2000/09/xmldsig#");
				break;
			}
		}
	}

	public override void LoadInput(object obj)
	{
		XmlResolver resolver = (base.ResolverSet ? _xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), base.BaseURI));
		if (obj is Stream)
		{
			_excCanonicalXml = new ExcCanonicalXml((Stream)obj, _includeComments, _inclusiveNamespacesPrefixList, resolver, base.BaseURI);
			return;
		}
		if (obj is XmlDocument)
		{
			_excCanonicalXml = new ExcCanonicalXml((XmlDocument)obj, _includeComments, _inclusiveNamespacesPrefixList, resolver);
			return;
		}
		if (obj is XmlNodeList)
		{
			_excCanonicalXml = new ExcCanonicalXml((XmlNodeList)obj, _includeComments, _inclusiveNamespacesPrefixList, resolver);
			return;
		}
		throw new ArgumentException("Type of input object is invalid.", "obj");
	}

	protected override XmlNodeList GetInnerXml()
	{
		if (InclusiveNamespacesPrefixList == null)
		{
			return null;
		}
		XmlDocument xmlDocument = new XmlDocument();
		XmlElement xmlElement = xmlDocument.CreateElement("Transform", "http://www.w3.org/2000/09/xmldsig#");
		if (!string.IsNullOrEmpty(base.Algorithm))
		{
			xmlElement.SetAttribute("Algorithm", base.Algorithm);
		}
		XmlElement xmlElement2 = xmlDocument.CreateElement("InclusiveNamespaces", "http://www.w3.org/2001/10/xml-exc-c14n#");
		xmlElement2.SetAttribute("PrefixList", InclusiveNamespacesPrefixList);
		xmlElement.AppendChild(xmlElement2);
		return xmlElement.ChildNodes;
	}

	public override object GetOutput()
	{
		return new MemoryStream(_excCanonicalXml.GetBytes());
	}

	public override object GetOutput(Type type)
	{
		if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream)))
		{
			throw new ArgumentException("The input type was invalid for this transform.", "type");
		}
		return new MemoryStream(_excCanonicalXml.GetBytes());
	}

	public override byte[] GetDigestedOutput(HashAlgorithm hash)
	{
		return _excCanonicalXml.GetDigestedBytes(hash);
	}
}

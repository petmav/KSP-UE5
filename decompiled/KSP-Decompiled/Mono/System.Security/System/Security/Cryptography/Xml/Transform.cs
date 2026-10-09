using System.Collections;
using System.IO;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public abstract class Transform
{
	private string _algorithm;

	private string _baseUri;

	internal XmlResolver _xmlResolver;

	private bool _bResolverSet;

	private SignedXml _signedXml;

	private Reference _reference;

	private Hashtable _propagatedNamespaces;

	private XmlElement _context;

	internal string BaseURI
	{
		get
		{
			return _baseUri;
		}
		set
		{
			_baseUri = value;
		}
	}

	internal SignedXml SignedXml
	{
		get
		{
			return _signedXml;
		}
		set
		{
			_signedXml = value;
		}
	}

	internal Reference Reference
	{
		get
		{
			return _reference;
		}
		set
		{
			_reference = value;
		}
	}

	public string Algorithm
	{
		get
		{
			return _algorithm;
		}
		set
		{
			_algorithm = value;
		}
	}

	public XmlResolver Resolver
	{
		internal get
		{
			return _xmlResolver;
		}
		set
		{
			_xmlResolver = value;
			_bResolverSet = true;
		}
	}

	internal bool ResolverSet => _bResolverSet;

	public abstract Type[] InputTypes { get; }

	public abstract Type[] OutputTypes { get; }

	public XmlElement Context
	{
		get
		{
			if (_context != null)
			{
				return _context;
			}
			Reference reference = Reference;
			return ((reference == null) ? SignedXml : reference.SignedXml)?._context;
		}
		set
		{
			_context = value;
		}
	}

	public Hashtable PropagatedNamespaces
	{
		get
		{
			if (_propagatedNamespaces != null)
			{
				return _propagatedNamespaces;
			}
			Reference reference = Reference;
			SignedXml signedXml = ((reference == null) ? SignedXml : reference.SignedXml);
			if (reference != null && (reference.ReferenceTargetType != ReferenceTargetType.UriReference || string.IsNullOrEmpty(reference.Uri) || reference.Uri[0] != '#'))
			{
				_propagatedNamespaces = new Hashtable(0);
				return _propagatedNamespaces;
			}
			CanonicalXmlNodeList canonicalXmlNodeList = null;
			if (reference != null)
			{
				canonicalXmlNodeList = reference._namespaces;
			}
			else if (signedXml?._context != null)
			{
				canonicalXmlNodeList = Utils.GetPropagatedAttributes(signedXml._context);
			}
			if (canonicalXmlNodeList == null)
			{
				_propagatedNamespaces = new Hashtable(0);
				return _propagatedNamespaces;
			}
			_propagatedNamespaces = new Hashtable(canonicalXmlNodeList.Count);
			foreach (XmlNode item in canonicalXmlNodeList)
			{
				string key = ((item.Prefix.Length > 0) ? (item.Prefix + ":" + item.LocalName) : item.LocalName);
				if (!_propagatedNamespaces.Contains(key))
				{
					_propagatedNamespaces.Add(key, item.Value);
				}
			}
			return _propagatedNamespaces;
		}
	}

	internal bool AcceptsType(Type inputType)
	{
		if (InputTypes != null)
		{
			for (int i = 0; i < InputTypes.Length; i++)
			{
				if (inputType == InputTypes[i] || inputType.IsSubclassOf(InputTypes[i]))
				{
					return true;
				}
			}
		}
		return false;
	}

	public XmlElement GetXml()
	{
		XmlDocument xmlDocument = new XmlDocument();
		xmlDocument.PreserveWhitespace = true;
		return GetXml(xmlDocument);
	}

	internal XmlElement GetXml(XmlDocument document)
	{
		return GetXml(document, "Transform");
	}

	internal XmlElement GetXml(XmlDocument document, string name)
	{
		XmlElement xmlElement = document.CreateElement(name, "http://www.w3.org/2000/09/xmldsig#");
		if (!string.IsNullOrEmpty(Algorithm))
		{
			xmlElement.SetAttribute("Algorithm", Algorithm);
		}
		XmlNodeList innerXml = GetInnerXml();
		if (innerXml != null)
		{
			foreach (XmlNode item in innerXml)
			{
				xmlElement.AppendChild(document.ImportNode(item, deep: true));
			}
		}
		return xmlElement;
	}

	public abstract void LoadInnerXml(XmlNodeList nodeList);

	protected abstract XmlNodeList GetInnerXml();

	public abstract void LoadInput(object obj);

	public abstract object GetOutput();

	public abstract object GetOutput(Type type);

	public virtual byte[] GetDigestedOutput(HashAlgorithm hash)
	{
		return hash.ComputeHash((Stream)GetOutput(typeof(Stream)));
	}
}

using System.Xml;

namespace System.Security.Cryptography.Xml;

public abstract class EncryptedReference
{
	private string _uri;

	private string _referenceType;

	private TransformChain _transformChain;

	internal XmlElement _cachedXml;

	public string Uri
	{
		get
		{
			return _uri;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("A Uri attribute is required for a CipherReference element.");
			}
			_uri = value;
			_cachedXml = null;
		}
	}

	public TransformChain TransformChain
	{
		get
		{
			if (_transformChain == null)
			{
				_transformChain = new TransformChain();
			}
			return _transformChain;
		}
		set
		{
			_transformChain = value;
			_cachedXml = null;
		}
	}

	protected string ReferenceType
	{
		get
		{
			return _referenceType;
		}
		set
		{
			_referenceType = value;
			_cachedXml = null;
		}
	}

	protected internal bool CacheValid => _cachedXml != null;

	protected EncryptedReference()
		: this(string.Empty, new TransformChain())
	{
	}

	protected EncryptedReference(string uri)
		: this(uri, new TransformChain())
	{
	}

	protected EncryptedReference(string uri, TransformChain transformChain)
	{
		TransformChain = transformChain;
		Uri = uri;
		_cachedXml = null;
	}

	public void AddTransform(Transform transform)
	{
		TransformChain.Add(transform);
	}

	public virtual XmlElement GetXml()
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
		if (ReferenceType == null)
		{
			throw new CryptographicException("The Reference type must be set in an EncryptedReference object.");
		}
		XmlElement xmlElement = document.CreateElement(ReferenceType, "http://www.w3.org/2001/04/xmlenc#");
		if (!string.IsNullOrEmpty(_uri))
		{
			xmlElement.SetAttribute("URI", _uri);
		}
		if (TransformChain.Count > 0)
		{
			xmlElement.AppendChild(TransformChain.GetXml(document, "http://www.w3.org/2000/09/xmldsig#"));
		}
		return xmlElement;
	}

	public virtual void LoadXml(XmlElement value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		ReferenceType = value.LocalName;
		Uri = Utils.GetAttribute(value, "URI", "http://www.w3.org/2001/04/xmlenc#");
		XmlNamespaceManager xmlNamespaceManager = new XmlNamespaceManager(value.OwnerDocument.NameTable);
		xmlNamespaceManager.AddNamespace("ds", "http://www.w3.org/2000/09/xmldsig#");
		XmlNode xmlNode = value.SelectSingleNode("ds:Transforms", xmlNamespaceManager);
		if (xmlNode != null)
		{
			TransformChain.LoadXml(xmlNode as XmlElement);
		}
		_cachedXml = value;
	}
}

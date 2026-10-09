using System.IO;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public class XmlDsigEnvelopedSignatureTransform : Transform
{
	private Type[] _inputTypes = new Type[3]
	{
		typeof(Stream),
		typeof(XmlNodeList),
		typeof(XmlDocument)
	};

	private Type[] _outputTypes = new Type[2]
	{
		typeof(XmlNodeList),
		typeof(XmlDocument)
	};

	private XmlNodeList _inputNodeList;

	private bool _includeComments;

	private XmlNamespaceManager _nsm;

	private XmlDocument _containingDocument;

	private int _signaturePosition;

	internal int SignaturePosition
	{
		set
		{
			_signaturePosition = value;
		}
	}

	public override Type[] InputTypes => _inputTypes;

	public override Type[] OutputTypes => _outputTypes;

	public XmlDsigEnvelopedSignatureTransform()
	{
		base.Algorithm = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";
	}

	public XmlDsigEnvelopedSignatureTransform(bool includeComments)
	{
		_includeComments = includeComments;
		base.Algorithm = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";
	}

	public override void LoadInnerXml(XmlNodeList nodeList)
	{
	}

	protected override XmlNodeList GetInnerXml()
	{
		return null;
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
		XmlDocument xmlDocument = new XmlDocument();
		xmlDocument.PreserveWhitespace = true;
		XmlResolver xmlResolver = (base.ResolverSet ? _xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), base.BaseURI));
		XmlReader reader = Utils.PreProcessStreamInput(stream, xmlResolver, base.BaseURI);
		xmlDocument.Load(reader);
		_containingDocument = xmlDocument;
		if (_containingDocument == null)
		{
			throw new CryptographicException("An XmlDocument context is required for enveloped transforms.");
		}
		_nsm = new XmlNamespaceManager(_containingDocument.NameTable);
		_nsm.AddNamespace("dsig", "http://www.w3.org/2000/09/xmldsig#");
	}

	private void LoadXmlNodeListInput(XmlNodeList nodeList)
	{
		if (nodeList == null)
		{
			throw new ArgumentNullException("nodeList");
		}
		_containingDocument = Utils.GetOwnerDocument(nodeList);
		if (_containingDocument == null)
		{
			throw new CryptographicException("An XmlDocument context is required for enveloped transforms.");
		}
		_nsm = new XmlNamespaceManager(_containingDocument.NameTable);
		_nsm.AddNamespace("dsig", "http://www.w3.org/2000/09/xmldsig#");
		_inputNodeList = nodeList;
	}

	private void LoadXmlDocumentInput(XmlDocument doc)
	{
		if (doc == null)
		{
			throw new ArgumentNullException("doc");
		}
		_containingDocument = doc;
		_nsm = new XmlNamespaceManager(_containingDocument.NameTable);
		_nsm.AddNamespace("dsig", "http://www.w3.org/2000/09/xmldsig#");
	}

	public override object GetOutput()
	{
		if (_containingDocument == null)
		{
			throw new CryptographicException("An XmlDocument context is required for enveloped transforms.");
		}
		if (_inputNodeList != null)
		{
			if (_signaturePosition == 0)
			{
				return _inputNodeList;
			}
			XmlNodeList xmlNodeList = _containingDocument.SelectNodes("//dsig:Signature", _nsm);
			if (xmlNodeList == null)
			{
				return _inputNodeList;
			}
			CanonicalXmlNodeList canonicalXmlNodeList = new CanonicalXmlNodeList();
			{
				foreach (XmlNode inputNode in _inputNodeList)
				{
					if (inputNode == null)
					{
						continue;
					}
					if (Utils.IsXmlNamespaceNode(inputNode) || Utils.IsNamespaceNode(inputNode))
					{
						canonicalXmlNodeList.Add(inputNode);
						continue;
					}
					try
					{
						XmlNode xmlNode2 = inputNode.SelectSingleNode("ancestor-or-self::dsig:Signature[1]", _nsm);
						int num = 0;
						foreach (XmlNode item in xmlNodeList)
						{
							num++;
							if (item == xmlNode2)
							{
								break;
							}
						}
						if (xmlNode2 == null || (xmlNode2 != null && num != _signaturePosition))
						{
							canonicalXmlNodeList.Add(inputNode);
						}
					}
					catch
					{
					}
				}
				return canonicalXmlNodeList;
			}
		}
		XmlNodeList xmlNodeList2 = _containingDocument.SelectNodes("//dsig:Signature", _nsm);
		if (xmlNodeList2 == null)
		{
			return _containingDocument;
		}
		if (xmlNodeList2.Count < _signaturePosition || _signaturePosition <= 0)
		{
			return _containingDocument;
		}
		xmlNodeList2[_signaturePosition - 1].ParentNode.RemoveChild(xmlNodeList2[_signaturePosition - 1]);
		return _containingDocument;
	}

	public override object GetOutput(Type type)
	{
		if (type == typeof(XmlNodeList) || type.IsSubclassOf(typeof(XmlNodeList)))
		{
			if (_inputNodeList == null)
			{
				_inputNodeList = Utils.AllDescendantNodes(_containingDocument, includeComments: true);
			}
			return (XmlNodeList)GetOutput();
		}
		if (type == typeof(XmlDocument) || type.IsSubclassOf(typeof(XmlDocument)))
		{
			if (_inputNodeList != null)
			{
				throw new ArgumentException("The input type was invalid for this transform.", "type");
			}
			return (XmlDocument)GetOutput();
		}
		throw new ArgumentException("The input type was invalid for this transform.", "type");
	}
}

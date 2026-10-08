using System.Collections;
using System.IO;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public class XmlDecryptionTransform : Transform
{
	private Type[] _inputTypes = new Type[2]
	{
		typeof(Stream),
		typeof(XmlDocument)
	};

	private Type[] _outputTypes = new Type[1] { typeof(XmlDocument) };

	private XmlNodeList _encryptedDataList;

	private ArrayList _arrayListUri;

	private EncryptedXml _exml;

	private XmlDocument _containingDocument;

	private XmlNamespaceManager _nsm;

	private const string XmlDecryptionTransformNamespaceUrl = "http://www.w3.org/2002/07/decrypt#";

	private ArrayList ExceptUris
	{
		get
		{
			if (_arrayListUri == null)
			{
				_arrayListUri = new ArrayList();
			}
			return _arrayListUri;
		}
	}

	public EncryptedXml EncryptedXml
	{
		get
		{
			if (_exml != null)
			{
				return _exml;
			}
			Reference reference = base.Reference;
			SignedXml signedXml = ((reference == null) ? base.SignedXml : reference.SignedXml);
			if (signedXml == null || signedXml.EncryptedXml == null)
			{
				_exml = new EncryptedXml(_containingDocument);
			}
			else
			{
				_exml = signedXml.EncryptedXml;
			}
			return _exml;
		}
		set
		{
			_exml = value;
		}
	}

	public override Type[] InputTypes => _inputTypes;

	public override Type[] OutputTypes => _outputTypes;

	public XmlDecryptionTransform()
	{
		base.Algorithm = "http://www.w3.org/2002/07/decrypt#XML";
	}

	protected virtual bool IsTargetElement(XmlElement inputElement, string idValue)
	{
		if (inputElement == null)
		{
			return false;
		}
		if (inputElement.GetAttribute("Id") == idValue || inputElement.GetAttribute("id") == idValue || inputElement.GetAttribute("ID") == idValue)
		{
			return true;
		}
		return false;
	}

	public void AddExceptUri(string uri)
	{
		if (uri == null)
		{
			throw new ArgumentNullException("uri");
		}
		ExceptUris.Add(uri);
	}

	public override void LoadInnerXml(XmlNodeList nodeList)
	{
		if (nodeList == null)
		{
			throw new CryptographicException("Unknown transform has been encountered.");
		}
		ExceptUris.Clear();
		foreach (XmlNode node in nodeList)
		{
			if (node is XmlElement { LocalName: "Except", NamespaceURI: "http://www.w3.org/2002/07/decrypt#" } xmlElement)
			{
				string attribute = Utils.GetAttribute(xmlElement, "URI", "http://www.w3.org/2002/07/decrypt#");
				if (attribute == null || attribute.Length == 0 || attribute[0] != '#')
				{
					throw new CryptographicException("A Uri attribute is required for a CipherReference element.");
				}
				string value = Utils.ExtractIdFromLocalUri(attribute);
				ExceptUris.Add(value);
			}
		}
	}

	protected override XmlNodeList GetInnerXml()
	{
		if (ExceptUris.Count == 0)
		{
			return null;
		}
		XmlDocument xmlDocument = new XmlDocument();
		XmlElement xmlElement = xmlDocument.CreateElement("Transform", "http://www.w3.org/2000/09/xmldsig#");
		if (!string.IsNullOrEmpty(base.Algorithm))
		{
			xmlElement.SetAttribute("Algorithm", base.Algorithm);
		}
		foreach (string exceptUri in ExceptUris)
		{
			XmlElement xmlElement2 = xmlDocument.CreateElement("Except", "http://www.w3.org/2002/07/decrypt#");
			xmlElement2.SetAttribute("URI", exceptUri);
			xmlElement.AppendChild(xmlElement2);
		}
		return xmlElement.ChildNodes;
	}

	public override void LoadInput(object obj)
	{
		if (obj is Stream)
		{
			LoadStreamInput((Stream)obj);
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
		_nsm = new XmlNamespaceManager(_containingDocument.NameTable);
		_nsm.AddNamespace("enc", "http://www.w3.org/2001/04/xmlenc#");
		_encryptedDataList = xmlDocument.SelectNodes("//enc:EncryptedData", _nsm);
	}

	private void LoadXmlDocumentInput(XmlDocument document)
	{
		if (document == null)
		{
			throw new ArgumentNullException("document");
		}
		_containingDocument = document;
		_nsm = new XmlNamespaceManager(document.NameTable);
		_nsm.AddNamespace("enc", "http://www.w3.org/2001/04/xmlenc#");
		_encryptedDataList = document.SelectNodes("//enc:EncryptedData", _nsm);
	}

	private void ReplaceEncryptedData(XmlElement encryptedDataElement, byte[] decrypted)
	{
		XmlNode parentNode = encryptedDataElement.ParentNode;
		if (parentNode.NodeType == XmlNodeType.Document)
		{
			parentNode.InnerXml = EncryptedXml.Encoding.GetString(decrypted);
		}
		else
		{
			EncryptedXml.ReplaceData(encryptedDataElement, decrypted);
		}
	}

	private bool ProcessEncryptedDataItem(XmlElement encryptedDataElement)
	{
		if (ExceptUris.Count > 0)
		{
			for (int i = 0; i < ExceptUris.Count; i++)
			{
				if (IsTargetElement(encryptedDataElement, (string)ExceptUris[i]))
				{
					return false;
				}
			}
		}
		EncryptedData encryptedData = new EncryptedData();
		encryptedData.LoadXml(encryptedDataElement);
		SymmetricAlgorithm decryptionKey = EncryptedXml.GetDecryptionKey(encryptedData, null);
		if (decryptionKey == null)
		{
			throw new CryptographicException("Unable to retrieve the decryption key.");
		}
		byte[] decrypted = EncryptedXml.DecryptData(encryptedData, decryptionKey);
		ReplaceEncryptedData(encryptedDataElement, decrypted);
		return true;
	}

	private void ProcessElementRecursively(XmlNodeList encryptedDatas)
	{
		if (encryptedDatas == null || encryptedDatas.Count == 0)
		{
			return;
		}
		Queue queue = new Queue();
		foreach (XmlNode encryptedData in encryptedDatas)
		{
			queue.Enqueue(encryptedData);
		}
		XmlNode xmlNode = queue.Dequeue() as XmlNode;
		while (xmlNode != null)
		{
			if (xmlNode is XmlElement { LocalName: "EncryptedData", NamespaceURI: "http://www.w3.org/2001/04/xmlenc#", NextSibling: var nextSibling, ParentNode: var parentNode } xmlElement && ProcessEncryptedDataItem(xmlElement))
			{
				XmlNode xmlNode2 = parentNode.FirstChild;
				while (xmlNode2 != null && xmlNode2.NextSibling != nextSibling)
				{
					xmlNode2 = xmlNode2.NextSibling;
				}
				if (xmlNode2 != null)
				{
					XmlNodeList xmlNodeList = xmlNode2.SelectNodes("//enc:EncryptedData", _nsm);
					if (xmlNodeList.Count > 0)
					{
						foreach (XmlNode item in xmlNodeList)
						{
							queue.Enqueue(item);
						}
					}
				}
			}
			if (queue.Count != 0)
			{
				xmlNode = queue.Dequeue() as XmlNode;
				continue;
			}
			break;
		}
	}

	public override object GetOutput()
	{
		if (_encryptedDataList != null)
		{
			ProcessElementRecursively(_encryptedDataList);
		}
		Utils.AddNamespaces(_containingDocument.DocumentElement, base.PropagatedNamespaces);
		return _containingDocument;
	}

	public override object GetOutput(Type type)
	{
		if (type == typeof(XmlDocument))
		{
			return (XmlDocument)GetOutput();
		}
		throw new ArgumentException("The input type was invalid for this transform.", "type");
	}
}

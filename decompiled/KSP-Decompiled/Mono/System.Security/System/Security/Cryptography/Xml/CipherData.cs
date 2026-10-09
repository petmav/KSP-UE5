using System.Xml;

namespace System.Security.Cryptography.Xml;

public sealed class CipherData
{
	private XmlElement _cachedXml;

	private CipherReference _cipherReference;

	private byte[] _cipherValue;

	private bool CacheValid => _cachedXml != null;

	public CipherReference CipherReference
	{
		get
		{
			return _cipherReference;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			if (CipherValue != null)
			{
				throw new CryptographicException("A Cipher Data element should have either a CipherValue or a CipherReference element.");
			}
			_cipherReference = value;
			_cachedXml = null;
		}
	}

	public byte[] CipherValue
	{
		get
		{
			return _cipherValue;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			if (CipherReference != null)
			{
				throw new CryptographicException("A Cipher Data element should have either a CipherValue or a CipherReference element.");
			}
			_cipherValue = (byte[])value.Clone();
			_cachedXml = null;
		}
	}

	public CipherData()
	{
	}

	public CipherData(byte[] cipherValue)
	{
		CipherValue = cipherValue;
	}

	public CipherData(CipherReference cipherReference)
	{
		CipherReference = cipherReference;
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
		XmlElement xmlElement = document.CreateElement("CipherData", "http://www.w3.org/2001/04/xmlenc#");
		if (CipherValue != null)
		{
			XmlElement xmlElement2 = document.CreateElement("CipherValue", "http://www.w3.org/2001/04/xmlenc#");
			xmlElement2.AppendChild(document.CreateTextNode(Convert.ToBase64String(CipherValue)));
			xmlElement.AppendChild(xmlElement2);
		}
		else
		{
			if (CipherReference == null)
			{
				throw new CryptographicException("A Cipher Data element should have either a CipherValue or a CipherReference element.");
			}
			xmlElement.AppendChild(CipherReference.GetXml(document));
		}
		return xmlElement;
	}

	public void LoadXml(XmlElement value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		XmlNamespaceManager xmlNamespaceManager = new XmlNamespaceManager(value.OwnerDocument.NameTable);
		xmlNamespaceManager.AddNamespace("enc", "http://www.w3.org/2001/04/xmlenc#");
		XmlNode xmlNode = value.SelectSingleNode("enc:CipherValue", xmlNamespaceManager);
		XmlNode xmlNode2 = value.SelectSingleNode("enc:CipherReference", xmlNamespaceManager);
		if (xmlNode != null)
		{
			if (xmlNode2 != null)
			{
				throw new CryptographicException("A Cipher Data element should have either a CipherValue or a CipherReference element.");
			}
			_cipherValue = Convert.FromBase64String(Utils.DiscardWhiteSpaces(xmlNode.InnerText));
		}
		else
		{
			if (xmlNode2 == null)
			{
				throw new CryptographicException("A Cipher Data element should have either a CipherValue or a CipherReference element.");
			}
			_cipherReference = new CipherReference();
			_cipherReference.LoadXml((XmlElement)xmlNode2);
		}
		_cachedXml = value;
	}
}

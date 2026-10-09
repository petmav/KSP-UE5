using System.Xml;

namespace System.Security.Cryptography.Xml;

public sealed class EncryptedData : EncryptedType
{
	public override void LoadXml(XmlElement value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		XmlNamespaceManager xmlNamespaceManager = new XmlNamespaceManager(value.OwnerDocument.NameTable);
		xmlNamespaceManager.AddNamespace("enc", "http://www.w3.org/2001/04/xmlenc#");
		xmlNamespaceManager.AddNamespace("ds", "http://www.w3.org/2000/09/xmldsig#");
		Id = Utils.GetAttribute(value, "Id", "http://www.w3.org/2001/04/xmlenc#");
		Type = Utils.GetAttribute(value, "Type", "http://www.w3.org/2001/04/xmlenc#");
		MimeType = Utils.GetAttribute(value, "MimeType", "http://www.w3.org/2001/04/xmlenc#");
		Encoding = Utils.GetAttribute(value, "Encoding", "http://www.w3.org/2001/04/xmlenc#");
		XmlNode xmlNode = value.SelectSingleNode("enc:EncryptionMethod", xmlNamespaceManager);
		EncryptionMethod = new EncryptionMethod();
		if (xmlNode != null)
		{
			EncryptionMethod.LoadXml(xmlNode as XmlElement);
		}
		base.KeyInfo = new KeyInfo();
		XmlNode xmlNode2 = value.SelectSingleNode("ds:KeyInfo", xmlNamespaceManager);
		if (xmlNode2 != null)
		{
			base.KeyInfo.LoadXml(xmlNode2 as XmlElement);
		}
		XmlNode xmlNode3 = value.SelectSingleNode("enc:CipherData", xmlNamespaceManager);
		if (xmlNode3 == null)
		{
			throw new CryptographicException("Cipher data is not specified.");
		}
		CipherData = new CipherData();
		CipherData.LoadXml(xmlNode3 as XmlElement);
		XmlNode xmlNode4 = value.SelectSingleNode("enc:EncryptionProperties", xmlNamespaceManager);
		if (xmlNode4 != null)
		{
			XmlNodeList xmlNodeList = xmlNode4.SelectNodes("enc:EncryptionProperty", xmlNamespaceManager);
			if (xmlNodeList != null)
			{
				foreach (XmlNode item in xmlNodeList)
				{
					EncryptionProperty encryptionProperty = new EncryptionProperty();
					encryptionProperty.LoadXml(item as XmlElement);
					EncryptionProperties.Add(encryptionProperty);
				}
			}
		}
		_cachedXml = value;
	}

	public override XmlElement GetXml()
	{
		if (base.CacheValid)
		{
			return _cachedXml;
		}
		XmlDocument xmlDocument = new XmlDocument();
		xmlDocument.PreserveWhitespace = true;
		return GetXml(xmlDocument);
	}

	internal XmlElement GetXml(XmlDocument document)
	{
		XmlElement xmlElement = document.CreateElement("EncryptedData", "http://www.w3.org/2001/04/xmlenc#");
		if (!string.IsNullOrEmpty(Id))
		{
			xmlElement.SetAttribute("Id", Id);
		}
		if (!string.IsNullOrEmpty(Type))
		{
			xmlElement.SetAttribute("Type", Type);
		}
		if (!string.IsNullOrEmpty(MimeType))
		{
			xmlElement.SetAttribute("MimeType", MimeType);
		}
		if (!string.IsNullOrEmpty(Encoding))
		{
			xmlElement.SetAttribute("Encoding", Encoding);
		}
		if (EncryptionMethod != null)
		{
			xmlElement.AppendChild(EncryptionMethod.GetXml(document));
		}
		if (base.KeyInfo.Count > 0)
		{
			xmlElement.AppendChild(base.KeyInfo.GetXml(document));
		}
		if (CipherData == null)
		{
			throw new CryptographicException("Cipher data is not specified.");
		}
		xmlElement.AppendChild(CipherData.GetXml(document));
		if (EncryptionProperties.Count > 0)
		{
			XmlElement xmlElement2 = document.CreateElement("EncryptionProperties", "http://www.w3.org/2001/04/xmlenc#");
			for (int i = 0; i < EncryptionProperties.Count; i++)
			{
				EncryptionProperty encryptionProperty = EncryptionProperties.Item(i);
				xmlElement2.AppendChild(encryptionProperty.GetXml(document));
			}
			xmlElement.AppendChild(xmlElement2);
		}
		return xmlElement;
	}
}

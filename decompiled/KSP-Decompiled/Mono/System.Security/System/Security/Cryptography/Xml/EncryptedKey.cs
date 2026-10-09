using System.Xml;

namespace System.Security.Cryptography.Xml;

public sealed class EncryptedKey : EncryptedType
{
	private string _recipient;

	private string _carriedKeyName;

	private ReferenceList _referenceList;

	public string Recipient
	{
		get
		{
			if (_recipient == null)
			{
				_recipient = string.Empty;
			}
			return _recipient;
		}
		set
		{
			_recipient = value;
			_cachedXml = null;
		}
	}

	public string CarriedKeyName
	{
		get
		{
			return _carriedKeyName;
		}
		set
		{
			_carriedKeyName = value;
			_cachedXml = null;
		}
	}

	public ReferenceList ReferenceList
	{
		get
		{
			if (_referenceList == null)
			{
				_referenceList = new ReferenceList();
			}
			return _referenceList;
		}
	}

	public void AddReference(DataReference dataReference)
	{
		ReferenceList.Add(dataReference);
	}

	public void AddReference(KeyReference keyReference)
	{
		ReferenceList.Add(keyReference);
	}

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
		Recipient = Utils.GetAttribute(value, "Recipient", "http://www.w3.org/2001/04/xmlenc#");
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
		XmlNode xmlNode6 = value.SelectSingleNode("enc:CarriedKeyName", xmlNamespaceManager);
		if (xmlNode6 != null)
		{
			CarriedKeyName = xmlNode6.InnerText;
		}
		XmlNode xmlNode7 = value.SelectSingleNode("enc:ReferenceList", xmlNamespaceManager);
		if (xmlNode7 != null)
		{
			XmlNodeList xmlNodeList2 = xmlNode7.SelectNodes("enc:DataReference", xmlNamespaceManager);
			if (xmlNodeList2 != null)
			{
				foreach (XmlNode item2 in xmlNodeList2)
				{
					DataReference dataReference = new DataReference();
					dataReference.LoadXml(item2 as XmlElement);
					ReferenceList.Add(dataReference);
				}
			}
			XmlNodeList xmlNodeList3 = xmlNode7.SelectNodes("enc:KeyReference", xmlNamespaceManager);
			if (xmlNodeList3 != null)
			{
				foreach (XmlNode item3 in xmlNodeList3)
				{
					KeyReference keyReference = new KeyReference();
					keyReference.LoadXml(item3 as XmlElement);
					ReferenceList.Add(keyReference);
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
		XmlElement xmlElement = document.CreateElement("EncryptedKey", "http://www.w3.org/2001/04/xmlenc#");
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
		if (!string.IsNullOrEmpty(Recipient))
		{
			xmlElement.SetAttribute("Recipient", Recipient);
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
		if (ReferenceList.Count > 0)
		{
			XmlElement xmlElement3 = document.CreateElement("ReferenceList", "http://www.w3.org/2001/04/xmlenc#");
			for (int j = 0; j < ReferenceList.Count; j++)
			{
				xmlElement3.AppendChild(ReferenceList[j].GetXml(document));
			}
			xmlElement.AppendChild(xmlElement3);
		}
		if (CarriedKeyName != null)
		{
			XmlElement xmlElement4 = document.CreateElement("CarriedKeyName", "http://www.w3.org/2001/04/xmlenc#");
			XmlText newChild = document.CreateTextNode(CarriedKeyName);
			xmlElement4.AppendChild(newChild);
			xmlElement.AppendChild(xmlElement4);
		}
		return xmlElement;
	}
}

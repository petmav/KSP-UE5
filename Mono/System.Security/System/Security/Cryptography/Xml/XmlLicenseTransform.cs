using System.IO;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public class XmlLicenseTransform : Transform
{
	private Type[] _inputTypes = new Type[1] { typeof(XmlDocument) };

	private Type[] _outputTypes = new Type[1] { typeof(XmlDocument) };

	private XmlNamespaceManager _namespaceManager;

	private XmlDocument _license;

	private IRelDecryptor _relDecryptor;

	private const string ElementIssuer = "issuer";

	private const string NamespaceUriCore = "urn:mpeg:mpeg21:2003:01-REL-R-NS";

	public override Type[] InputTypes => _inputTypes;

	public override Type[] OutputTypes => _outputTypes;

	public IRelDecryptor Decryptor
	{
		get
		{
			return _relDecryptor;
		}
		set
		{
			_relDecryptor = value;
		}
	}

	public XmlLicenseTransform()
	{
		base.Algorithm = "urn:mpeg:mpeg21:2003:01-REL-R-NS:licenseTransform";
	}

	private void DecryptEncryptedGrants(XmlNodeList encryptedGrantList, IRelDecryptor decryptor)
	{
		XmlElement xmlElement = null;
		XmlElement xmlElement2 = null;
		XmlElement xmlElement3 = null;
		EncryptionMethod encryptionMethod = null;
		KeyInfo keyInfo = null;
		CipherData cipherData = null;
		int i = 0;
		for (int count = encryptedGrantList.Count; i < count; i++)
		{
			xmlElement = encryptedGrantList[i].SelectSingleNode("//r:encryptedGrant/enc:EncryptionMethod", _namespaceManager) as XmlElement;
			xmlElement2 = encryptedGrantList[i].SelectSingleNode("//r:encryptedGrant/dsig:KeyInfo", _namespaceManager) as XmlElement;
			xmlElement3 = encryptedGrantList[i].SelectSingleNode("//r:encryptedGrant/enc:CipherData", _namespaceManager) as XmlElement;
			if (xmlElement != null && xmlElement2 != null && xmlElement3 != null)
			{
				encryptionMethod = new EncryptionMethod();
				keyInfo = new KeyInfo();
				cipherData = new CipherData();
				encryptionMethod.LoadXml(xmlElement);
				keyInfo.LoadXml(xmlElement2);
				cipherData.LoadXml(xmlElement3);
				MemoryStream memoryStream = null;
				Stream stream = null;
				StreamReader streamReader = null;
				try
				{
					memoryStream = new MemoryStream(cipherData.CipherValue);
					stream = _relDecryptor.Decrypt(encryptionMethod, keyInfo, memoryStream);
					if (stream == null || stream.Length == 0L)
					{
						throw new CryptographicException("Unable to decrypt grant content.");
					}
					streamReader = new StreamReader(stream);
					string innerXml = streamReader.ReadToEnd();
					encryptedGrantList[i].ParentNode.InnerXml = innerXml;
				}
				finally
				{
					memoryStream?.Close();
					stream?.Close();
					streamReader?.Close();
				}
				encryptionMethod = null;
				keyInfo = null;
				cipherData = null;
			}
			xmlElement = null;
			xmlElement2 = null;
			xmlElement3 = null;
		}
	}

	protected override XmlNodeList GetInnerXml()
	{
		return null;
	}

	public override object GetOutput()
	{
		return _license;
	}

	public override object GetOutput(Type type)
	{
		if (type != typeof(XmlDocument) && !type.IsSubclassOf(typeof(XmlDocument)))
		{
			throw new ArgumentException("The input type was invalid for this transform.", "type");
		}
		return GetOutput();
	}

	public override void LoadInnerXml(XmlNodeList nodeList)
	{
	}

	public override void LoadInput(object obj)
	{
		if (base.Context == null)
		{
			throw new CryptographicException("Null Context property encountered.");
		}
		_license = new XmlDocument();
		_license.PreserveWhitespace = true;
		_namespaceManager = new XmlNamespaceManager(_license.NameTable);
		_namespaceManager.AddNamespace("dsig", "http://www.w3.org/2000/09/xmldsig#");
		_namespaceManager.AddNamespace("enc", "http://www.w3.org/2001/04/xmlenc#");
		_namespaceManager.AddNamespace("r", "urn:mpeg:mpeg21:2003:01-REL-R-NS");
		XmlElement xmlElement = null;
		XmlElement xmlElement2 = null;
		XmlNode xmlNode = null;
		if (!(base.Context.SelectSingleNode("ancestor-or-self::r:issuer[1]", _namespaceManager) is XmlElement xmlElement3))
		{
			throw new CryptographicException("Issuer node is required.");
		}
		xmlNode = xmlElement3.SelectSingleNode("descendant-or-self::dsig:Signature[1]", _namespaceManager) as XmlElement;
		xmlNode?.ParentNode.RemoveChild(xmlNode);
		if (!(xmlElement3.SelectSingleNode("ancestor-or-self::r:license[1]", _namespaceManager) is XmlElement xmlElement4))
		{
			throw new CryptographicException("License node is required.");
		}
		XmlNodeList xmlNodeList = xmlElement4.SelectNodes("descendant-or-self::r:license[1]/r:issuer", _namespaceManager);
		int i = 0;
		for (int count = xmlNodeList.Count; i < count; i++)
		{
			if (xmlNodeList[i] != xmlElement3 && xmlNodeList[i].LocalName == "issuer" && xmlNodeList[i].NamespaceURI == "urn:mpeg:mpeg21:2003:01-REL-R-NS")
			{
				xmlNodeList[i].ParentNode.RemoveChild(xmlNodeList[i]);
			}
		}
		XmlNodeList xmlNodeList2 = xmlElement4.SelectNodes("/r:license/r:grant/r:encryptedGrant", _namespaceManager);
		if (xmlNodeList2.Count > 0)
		{
			if (_relDecryptor == null)
			{
				throw new CryptographicException("IRelDecryptor is required.");
			}
			DecryptEncryptedGrants(xmlNodeList2, _relDecryptor);
		}
		_license.InnerXml = xmlElement4.OuterXml;
	}
}

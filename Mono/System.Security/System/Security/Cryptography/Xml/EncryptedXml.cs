using System.Collections;
using System.IO;
using System.Net;
using System.Security.Cryptography.X509Certificates;
using System.Security.Policy;
using System.Text;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public class EncryptedXml
{
	public const string XmlEncNamespaceUrl = "http://www.w3.org/2001/04/xmlenc#";

	public const string XmlEncElementUrl = "http://www.w3.org/2001/04/xmlenc#Element";

	public const string XmlEncElementContentUrl = "http://www.w3.org/2001/04/xmlenc#Content";

	public const string XmlEncEncryptedKeyUrl = "http://www.w3.org/2001/04/xmlenc#EncryptedKey";

	public const string XmlEncDESUrl = "http://www.w3.org/2001/04/xmlenc#des-cbc";

	public const string XmlEncTripleDESUrl = "http://www.w3.org/2001/04/xmlenc#tripledes-cbc";

	public const string XmlEncAES128Url = "http://www.w3.org/2001/04/xmlenc#aes128-cbc";

	public const string XmlEncAES256Url = "http://www.w3.org/2001/04/xmlenc#aes256-cbc";

	public const string XmlEncAES192Url = "http://www.w3.org/2001/04/xmlenc#aes192-cbc";

	public const string XmlEncRSA15Url = "http://www.w3.org/2001/04/xmlenc#rsa-1_5";

	public const string XmlEncRSAOAEPUrl = "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p";

	public const string XmlEncTripleDESKeyWrapUrl = "http://www.w3.org/2001/04/xmlenc#kw-tripledes";

	public const string XmlEncAES128KeyWrapUrl = "http://www.w3.org/2001/04/xmlenc#kw-aes128";

	public const string XmlEncAES256KeyWrapUrl = "http://www.w3.org/2001/04/xmlenc#kw-aes256";

	public const string XmlEncAES192KeyWrapUrl = "http://www.w3.org/2001/04/xmlenc#kw-aes192";

	public const string XmlEncSHA256Url = "http://www.w3.org/2001/04/xmlenc#sha256";

	public const string XmlEncSHA512Url = "http://www.w3.org/2001/04/xmlenc#sha512";

	private XmlDocument _document;

	private Evidence _evidence;

	private XmlResolver _xmlResolver;

	private const int _capacity = 4;

	private Hashtable _keyNameMapping;

	private PaddingMode _padding;

	private CipherMode _mode;

	private Encoding _encoding;

	private string _recipient;

	private int _xmlDsigSearchDepthCounter;

	private int _xmlDsigSearchDepth;

	public int XmlDSigSearchDepth
	{
		get
		{
			return _xmlDsigSearchDepth;
		}
		set
		{
			_xmlDsigSearchDepth = value;
		}
	}

	public Evidence DocumentEvidence
	{
		get
		{
			return _evidence;
		}
		set
		{
			_evidence = value;
		}
	}

	public XmlResolver Resolver
	{
		get
		{
			return _xmlResolver;
		}
		set
		{
			_xmlResolver = value;
		}
	}

	public PaddingMode Padding
	{
		get
		{
			return _padding;
		}
		set
		{
			_padding = value;
		}
	}

	public CipherMode Mode
	{
		get
		{
			return _mode;
		}
		set
		{
			_mode = value;
		}
	}

	public Encoding Encoding
	{
		get
		{
			return _encoding;
		}
		set
		{
			_encoding = value;
		}
	}

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
		}
	}

	public EncryptedXml()
		: this(new XmlDocument())
	{
	}

	public EncryptedXml(XmlDocument document)
		: this(document, null)
	{
	}

	public EncryptedXml(XmlDocument document, Evidence evidence)
	{
		_document = document;
		_evidence = evidence;
		_xmlResolver = null;
		_padding = PaddingMode.ISO10126;
		_mode = CipherMode.CBC;
		_encoding = Encoding.UTF8;
		_keyNameMapping = new Hashtable(4);
		_xmlDsigSearchDepth = 20;
	}

	private bool IsOverXmlDsigRecursionLimit()
	{
		if (_xmlDsigSearchDepthCounter > XmlDSigSearchDepth)
		{
			return true;
		}
		return false;
	}

	private byte[] GetCipherValue(CipherData cipherData)
	{
		if (cipherData == null)
		{
			throw new ArgumentNullException("cipherData");
		}
		WebResponse webResponse = null;
		Stream stream = null;
		if (cipherData.CipherValue != null)
		{
			return cipherData.CipherValue;
		}
		if (cipherData.CipherReference != null)
		{
			if (cipherData.CipherReference.CipherValue != null)
			{
				return cipherData.CipherReference.CipherValue;
			}
			Stream stream2 = null;
			if (cipherData.CipherReference.Uri.Length == 0)
			{
				string baseUri = ((_document == null) ? null : _document.BaseURI);
				stream2 = cipherData.CipherReference.TransformChain.TransformToOctetStream(_document, _xmlResolver, baseUri);
			}
			else
			{
				if (cipherData.CipherReference.Uri[0] != '#')
				{
					throw new CryptographicException("Unable to resolve Uri {0}.", cipherData.CipherReference.Uri);
				}
				string idValue = Utils.ExtractIdFromLocalUri(cipherData.CipherReference.Uri);
				stream = new MemoryStream(_encoding.GetBytes(GetIdElement(_document, idValue).OuterXml));
				string baseUri2 = ((_document == null) ? null : _document.BaseURI);
				stream2 = cipherData.CipherReference.TransformChain.TransformToOctetStream(stream, _xmlResolver, baseUri2);
			}
			byte[] array = null;
			using (MemoryStream memoryStream = new MemoryStream())
			{
				Utils.Pump(stream2, memoryStream);
				array = memoryStream.ToArray();
				webResponse?.Close();
				stream?.Close();
				stream2.Close();
			}
			cipherData.CipherReference.CipherValue = array;
			return array;
		}
		throw new CryptographicException("Cipher data is not specified.");
	}

	public virtual XmlElement GetIdElement(XmlDocument document, string idValue)
	{
		return SignedXml.DefaultGetIdElement(document, idValue);
	}

	public virtual byte[] GetDecryptionIV(EncryptedData encryptedData, string symmetricAlgorithmUri)
	{
		if (encryptedData == null)
		{
			throw new ArgumentNullException("encryptedData");
		}
		int num = 0;
		if (symmetricAlgorithmUri == null)
		{
			if (encryptedData.EncryptionMethod == null)
			{
				throw new CryptographicException("Symmetric algorithm is not specified.");
			}
			symmetricAlgorithmUri = encryptedData.EncryptionMethod.KeyAlgorithm;
		}
		switch (symmetricAlgorithmUri)
		{
		case "http://www.w3.org/2001/04/xmlenc#des-cbc":
		case "http://www.w3.org/2001/04/xmlenc#tripledes-cbc":
			num = 8;
			break;
		case "http://www.w3.org/2001/04/xmlenc#aes128-cbc":
		case "http://www.w3.org/2001/04/xmlenc#aes192-cbc":
		case "http://www.w3.org/2001/04/xmlenc#aes256-cbc":
			num = 16;
			break;
		default:
			throw new CryptographicException(" The specified Uri is not supported.");
		}
		byte[] array = new byte[num];
		Buffer.BlockCopy(GetCipherValue(encryptedData.CipherData), 0, array, 0, array.Length);
		return array;
	}

	public virtual SymmetricAlgorithm GetDecryptionKey(EncryptedData encryptedData, string symmetricAlgorithmUri)
	{
		if (encryptedData == null)
		{
			throw new ArgumentNullException("encryptedData");
		}
		if (encryptedData.KeyInfo == null)
		{
			return null;
		}
		IEnumerator enumerator = encryptedData.KeyInfo.GetEnumerator();
		EncryptedKey encryptedKey = null;
		while (enumerator.MoveNext())
		{
			if (enumerator.Current is KeyInfoName { Value: var value })
			{
				if ((SymmetricAlgorithm)_keyNameMapping[value] != null)
				{
					return (SymmetricAlgorithm)_keyNameMapping[value];
				}
				XmlNamespaceManager xmlNamespaceManager = new XmlNamespaceManager(_document.NameTable);
				xmlNamespaceManager.AddNamespace("enc", "http://www.w3.org/2001/04/xmlenc#");
				XmlNodeList xmlNodeList = _document.SelectNodes("//enc:EncryptedKey", xmlNamespaceManager);
				if (xmlNodeList == null)
				{
					break;
				}
				foreach (XmlNode item in xmlNodeList)
				{
					XmlElement value2 = item as XmlElement;
					EncryptedKey encryptedKey2 = new EncryptedKey();
					encryptedKey2.LoadXml(value2);
					if (encryptedKey2.CarriedKeyName == value && encryptedKey2.Recipient == Recipient)
					{
						encryptedKey = encryptedKey2;
						break;
					}
				}
				break;
			}
			if (enumerator.Current is KeyInfoRetrievalMethod keyInfoRetrievalMethod)
			{
				string idValue = Utils.ExtractIdFromLocalUri(keyInfoRetrievalMethod.Uri);
				encryptedKey = new EncryptedKey();
				encryptedKey.LoadXml(GetIdElement(_document, idValue));
				break;
			}
			if (enumerator.Current is KeyInfoEncryptedKey keyInfoEncryptedKey)
			{
				encryptedKey = keyInfoEncryptedKey.EncryptedKey;
				break;
			}
		}
		if (encryptedKey != null)
		{
			if (symmetricAlgorithmUri == null)
			{
				if (encryptedData.EncryptionMethod == null)
				{
					throw new CryptographicException("Symmetric algorithm is not specified.");
				}
				symmetricAlgorithmUri = encryptedData.EncryptionMethod.KeyAlgorithm;
			}
			byte[] array = DecryptEncryptedKey(encryptedKey);
			if (array == null)
			{
				throw new CryptographicException("Unable to retrieve the decryption key.");
			}
			SymmetricAlgorithm obj = (SymmetricAlgorithm)CryptoHelpers.CreateFromName(symmetricAlgorithmUri);
			obj.Key = array;
			return obj;
		}
		return null;
	}

	public virtual byte[] DecryptEncryptedKey(EncryptedKey encryptedKey)
	{
		if (encryptedKey == null)
		{
			throw new ArgumentNullException("encryptedKey");
		}
		if (encryptedKey.KeyInfo == null)
		{
			return null;
		}
		IEnumerator enumerator = encryptedKey.KeyInfo.GetEnumerator();
		EncryptedKey encryptedKey2 = null;
		bool flag = false;
		while (enumerator.MoveNext())
		{
			if (enumerator.Current is KeyInfoName { Value: var value })
			{
				object obj = _keyNameMapping[value];
				if (obj == null)
				{
					break;
				}
				if (obj is SymmetricAlgorithm)
				{
					return DecryptKey(encryptedKey.CipherData.CipherValue, (SymmetricAlgorithm)obj);
				}
				flag = encryptedKey.EncryptionMethod != null && encryptedKey.EncryptionMethod.KeyAlgorithm == "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p";
				return DecryptKey(encryptedKey.CipherData.CipherValue, (RSA)obj, flag);
			}
			if (enumerator.Current is KeyInfoX509Data keyInfoX509Data)
			{
				X509Certificate2Enumerator enumerator2 = Utils.BuildBagOfCerts(keyInfoX509Data, CertUsageType.Decryption).GetEnumerator();
				while (enumerator2.MoveNext())
				{
					using RSA rSA = enumerator2.Current.GetRSAPrivateKey();
					if (rSA != null)
					{
						flag = encryptedKey.EncryptionMethod != null && encryptedKey.EncryptionMethod.KeyAlgorithm == "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p";
						return DecryptKey(encryptedKey.CipherData.CipherValue, rSA, flag);
					}
				}
				break;
			}
			if (enumerator.Current is KeyInfoRetrievalMethod keyInfoRetrievalMethod)
			{
				string idValue = Utils.ExtractIdFromLocalUri(keyInfoRetrievalMethod.Uri);
				encryptedKey2 = new EncryptedKey();
				encryptedKey2.LoadXml(GetIdElement(_document, idValue));
				try
				{
					_xmlDsigSearchDepthCounter++;
					if (IsOverXmlDsigRecursionLimit())
					{
						throw new CryptoSignedXmlRecursionException();
					}
					return DecryptEncryptedKey(encryptedKey2);
				}
				finally
				{
					_xmlDsigSearchDepthCounter--;
				}
			}
			if (enumerator.Current is KeyInfoEncryptedKey { EncryptedKey: var encryptedKey3 })
			{
				byte[] array = DecryptEncryptedKey(encryptedKey3);
				if (array != null)
				{
					SymmetricAlgorithm symmetricAlgorithm = (SymmetricAlgorithm)CryptoHelpers.CreateFromName(encryptedKey.EncryptionMethod.KeyAlgorithm);
					symmetricAlgorithm.Key = array;
					return DecryptKey(encryptedKey.CipherData.CipherValue, symmetricAlgorithm);
				}
			}
		}
		return null;
	}

	public void AddKeyNameMapping(string keyName, object keyObject)
	{
		if (keyName == null)
		{
			throw new ArgumentNullException("keyName");
		}
		if (keyObject == null)
		{
			throw new ArgumentNullException("keyObject");
		}
		if (!(keyObject is SymmetricAlgorithm) && !(keyObject is RSA))
		{
			throw new CryptographicException("The specified cryptographic transform is not supported.");
		}
		_keyNameMapping.Add(keyName, keyObject);
	}

	public void ClearKeyNameMappings()
	{
		_keyNameMapping.Clear();
	}

	public EncryptedData Encrypt(XmlElement inputElement, X509Certificate2 certificate)
	{
		if (inputElement == null)
		{
			throw new ArgumentNullException("inputElement");
		}
		if (certificate == null)
		{
			throw new ArgumentNullException("certificate");
		}
		using RSA rSA = certificate.GetRSAPublicKey();
		if (rSA == null)
		{
			throw new NotSupportedException("The certificate key algorithm is not supported.");
		}
		EncryptedData obj = new EncryptedData
		{
			Type = "http://www.w3.org/2001/04/xmlenc#Element",
			EncryptionMethod = new EncryptionMethod("http://www.w3.org/2001/04/xmlenc#aes256-cbc")
		};
		EncryptedKey encryptedKey = new EncryptedKey();
		encryptedKey.EncryptionMethod = new EncryptionMethod("http://www.w3.org/2001/04/xmlenc#rsa-1_5");
		encryptedKey.KeyInfo.AddClause(new KeyInfoX509Data(certificate));
		RijndaelManaged rijndaelManaged = new RijndaelManaged();
		encryptedKey.CipherData.CipherValue = EncryptKey(rijndaelManaged.Key, rSA, useOAEP: false);
		KeyInfoEncryptedKey clause = new KeyInfoEncryptedKey(encryptedKey);
		obj.KeyInfo.AddClause(clause);
		obj.CipherData.CipherValue = EncryptData(inputElement, rijndaelManaged, content: false);
		return obj;
	}

	public EncryptedData Encrypt(XmlElement inputElement, string keyName)
	{
		if (inputElement == null)
		{
			throw new ArgumentNullException("inputElement");
		}
		if (keyName == null)
		{
			throw new ArgumentNullException("keyName");
		}
		object obj = null;
		if (_keyNameMapping != null)
		{
			obj = _keyNameMapping[keyName];
		}
		if (obj == null)
		{
			throw new CryptographicException("Unable to retrieve the encryption key.");
		}
		SymmetricAlgorithm symmetricAlgorithm = obj as SymmetricAlgorithm;
		RSA rsa = obj as RSA;
		EncryptedData encryptedData = new EncryptedData();
		encryptedData.Type = "http://www.w3.org/2001/04/xmlenc#Element";
		encryptedData.EncryptionMethod = new EncryptionMethod("http://www.w3.org/2001/04/xmlenc#aes256-cbc");
		string algorithm = null;
		if (symmetricAlgorithm == null)
		{
			algorithm = "http://www.w3.org/2001/04/xmlenc#rsa-1_5";
		}
		else if (symmetricAlgorithm is TripleDES)
		{
			algorithm = "http://www.w3.org/2001/04/xmlenc#kw-tripledes";
		}
		else
		{
			if (!(symmetricAlgorithm is Rijndael) && !(symmetricAlgorithm is Aes))
			{
				throw new CryptographicException("The specified cryptographic transform is not supported.");
			}
			switch (symmetricAlgorithm.KeySize)
			{
			case 128:
				algorithm = "http://www.w3.org/2001/04/xmlenc#kw-aes128";
				break;
			case 192:
				algorithm = "http://www.w3.org/2001/04/xmlenc#kw-aes192";
				break;
			case 256:
				algorithm = "http://www.w3.org/2001/04/xmlenc#kw-aes256";
				break;
			}
		}
		EncryptedKey encryptedKey = new EncryptedKey();
		encryptedKey.EncryptionMethod = new EncryptionMethod(algorithm);
		encryptedKey.KeyInfo.AddClause(new KeyInfoName(keyName));
		RijndaelManaged rijndaelManaged = new RijndaelManaged();
		encryptedKey.CipherData.CipherValue = ((symmetricAlgorithm == null) ? EncryptKey(rijndaelManaged.Key, rsa, useOAEP: false) : EncryptKey(rijndaelManaged.Key, symmetricAlgorithm));
		KeyInfoEncryptedKey clause = new KeyInfoEncryptedKey(encryptedKey);
		encryptedData.KeyInfo.AddClause(clause);
		encryptedData.CipherData.CipherValue = EncryptData(inputElement, rijndaelManaged, content: false);
		return encryptedData;
	}

	public void DecryptDocument()
	{
		XmlNamespaceManager xmlNamespaceManager = new XmlNamespaceManager(_document.NameTable);
		xmlNamespaceManager.AddNamespace("enc", "http://www.w3.org/2001/04/xmlenc#");
		XmlNodeList xmlNodeList = _document.SelectNodes("//enc:EncryptedData", xmlNamespaceManager);
		if (xmlNodeList == null)
		{
			return;
		}
		foreach (XmlNode item in xmlNodeList)
		{
			XmlElement xmlElement = item as XmlElement;
			EncryptedData encryptedData = new EncryptedData();
			encryptedData.LoadXml(xmlElement);
			SymmetricAlgorithm decryptionKey = GetDecryptionKey(encryptedData, null);
			if (decryptionKey == null)
			{
				throw new CryptographicException("Unable to retrieve the decryption key.");
			}
			byte[] decryptedData = DecryptData(encryptedData, decryptionKey);
			ReplaceData(xmlElement, decryptedData);
		}
	}

	public byte[] EncryptData(byte[] plaintext, SymmetricAlgorithm symmetricAlgorithm)
	{
		if (plaintext == null)
		{
			throw new ArgumentNullException("plaintext");
		}
		if (symmetricAlgorithm == null)
		{
			throw new ArgumentNullException("symmetricAlgorithm");
		}
		CipherMode mode = symmetricAlgorithm.Mode;
		PaddingMode padding = symmetricAlgorithm.Padding;
		byte[] array = null;
		try
		{
			symmetricAlgorithm.Mode = _mode;
			symmetricAlgorithm.Padding = _padding;
			array = symmetricAlgorithm.CreateEncryptor().TransformFinalBlock(plaintext, 0, plaintext.Length);
		}
		finally
		{
			symmetricAlgorithm.Mode = mode;
			symmetricAlgorithm.Padding = padding;
		}
		byte[] array2 = null;
		if (_mode == CipherMode.ECB)
		{
			array2 = array;
		}
		else
		{
			byte[] iV = symmetricAlgorithm.IV;
			array2 = new byte[array.Length + iV.Length];
			Buffer.BlockCopy(iV, 0, array2, 0, iV.Length);
			Buffer.BlockCopy(array, 0, array2, iV.Length, array.Length);
		}
		return array2;
	}

	public byte[] EncryptData(XmlElement inputElement, SymmetricAlgorithm symmetricAlgorithm, bool content)
	{
		if (inputElement == null)
		{
			throw new ArgumentNullException("inputElement");
		}
		if (symmetricAlgorithm == null)
		{
			throw new ArgumentNullException("symmetricAlgorithm");
		}
		byte[] plaintext = (content ? _encoding.GetBytes(inputElement.InnerXml) : _encoding.GetBytes(inputElement.OuterXml));
		return EncryptData(plaintext, symmetricAlgorithm);
	}

	public byte[] DecryptData(EncryptedData encryptedData, SymmetricAlgorithm symmetricAlgorithm)
	{
		if (encryptedData == null)
		{
			throw new ArgumentNullException("encryptedData");
		}
		if (symmetricAlgorithm == null)
		{
			throw new ArgumentNullException("symmetricAlgorithm");
		}
		byte[] cipherValue = GetCipherValue(encryptedData.CipherData);
		CipherMode mode = symmetricAlgorithm.Mode;
		PaddingMode padding = symmetricAlgorithm.Padding;
		byte[] iV = symmetricAlgorithm.IV;
		byte[] array = null;
		if (_mode != CipherMode.ECB)
		{
			array = GetDecryptionIV(encryptedData, null);
		}
		byte[] array2 = null;
		try
		{
			int num = 0;
			if (array != null)
			{
				symmetricAlgorithm.IV = array;
				num = array.Length;
			}
			symmetricAlgorithm.Mode = _mode;
			symmetricAlgorithm.Padding = _padding;
			return symmetricAlgorithm.CreateDecryptor().TransformFinalBlock(cipherValue, num, cipherValue.Length - num);
		}
		finally
		{
			symmetricAlgorithm.Mode = mode;
			symmetricAlgorithm.Padding = padding;
			symmetricAlgorithm.IV = iV;
		}
	}

	public void ReplaceData(XmlElement inputElement, byte[] decryptedData)
	{
		if (inputElement == null)
		{
			throw new ArgumentNullException("inputElement");
		}
		if (decryptedData == null)
		{
			throw new ArgumentNullException("decryptedData");
		}
		XmlNode parentNode = inputElement.ParentNode;
		if (parentNode.NodeType == XmlNodeType.Document)
		{
			XmlDocument xmlDocument = new XmlDocument();
			xmlDocument.PreserveWhitespace = true;
			using (StringReader input = new StringReader(_encoding.GetString(decryptedData)))
			{
				using XmlReader reader = XmlReader.Create(input, Utils.GetSecureXmlReaderSettings(_xmlResolver));
				xmlDocument.Load(reader);
			}
			XmlNode newChild = inputElement.OwnerDocument.ImportNode(xmlDocument.DocumentElement, deep: true);
			parentNode.RemoveChild(inputElement);
			parentNode.AppendChild(newChild);
			return;
		}
		XmlNode xmlNode = parentNode.OwnerDocument.CreateElement(parentNode.Prefix, parentNode.LocalName, parentNode.NamespaceURI);
		try
		{
			parentNode.AppendChild(xmlNode);
			xmlNode.InnerXml = _encoding.GetString(decryptedData);
			XmlNode xmlNode2 = xmlNode.FirstChild;
			XmlNode nextSibling = inputElement.NextSibling;
			while (xmlNode2 != null)
			{
				XmlNode nextSibling2 = xmlNode2.NextSibling;
				parentNode.InsertBefore(xmlNode2, nextSibling);
				xmlNode2 = nextSibling2;
			}
		}
		finally
		{
			parentNode.RemoveChild(xmlNode);
		}
		parentNode.RemoveChild(inputElement);
	}

	public static void ReplaceElement(XmlElement inputElement, EncryptedData encryptedData, bool content)
	{
		if (inputElement == null)
		{
			throw new ArgumentNullException("inputElement");
		}
		if (encryptedData == null)
		{
			throw new ArgumentNullException("encryptedData");
		}
		XmlElement xml = encryptedData.GetXml(inputElement.OwnerDocument);
		switch (content)
		{
		case true:
			Utils.RemoveAllChildren(inputElement);
			inputElement.AppendChild(xml);
			break;
		case false:
			inputElement.ParentNode.ReplaceChild(xml, inputElement);
			break;
		}
	}

	public static byte[] EncryptKey(byte[] keyData, SymmetricAlgorithm symmetricAlgorithm)
	{
		if (keyData == null)
		{
			throw new ArgumentNullException("keyData");
		}
		if (symmetricAlgorithm == null)
		{
			throw new ArgumentNullException("symmetricAlgorithm");
		}
		if (symmetricAlgorithm is TripleDES)
		{
			return SymmetricKeyWrap.TripleDESKeyWrapEncrypt(symmetricAlgorithm.Key, keyData);
		}
		if (symmetricAlgorithm is Rijndael || symmetricAlgorithm is Aes)
		{
			return SymmetricKeyWrap.AESKeyWrapEncrypt(symmetricAlgorithm.Key, keyData);
		}
		throw new CryptographicException("The specified cryptographic transform is not supported.");
	}

	public static byte[] EncryptKey(byte[] keyData, RSA rsa, bool useOAEP)
	{
		if (keyData == null)
		{
			throw new ArgumentNullException("keyData");
		}
		if (rsa == null)
		{
			throw new ArgumentNullException("rsa");
		}
		if (useOAEP)
		{
			return new RSAOAEPKeyExchangeFormatter(rsa).CreateKeyExchange(keyData);
		}
		return new RSAPKCS1KeyExchangeFormatter(rsa).CreateKeyExchange(keyData);
	}

	public static byte[] DecryptKey(byte[] keyData, SymmetricAlgorithm symmetricAlgorithm)
	{
		if (keyData == null)
		{
			throw new ArgumentNullException("keyData");
		}
		if (symmetricAlgorithm == null)
		{
			throw new ArgumentNullException("symmetricAlgorithm");
		}
		if (symmetricAlgorithm is TripleDES)
		{
			return SymmetricKeyWrap.TripleDESKeyWrapDecrypt(symmetricAlgorithm.Key, keyData);
		}
		if (symmetricAlgorithm is Rijndael || symmetricAlgorithm is Aes)
		{
			return SymmetricKeyWrap.AESKeyWrapDecrypt(symmetricAlgorithm.Key, keyData);
		}
		throw new CryptographicException("The specified cryptographic transform is not supported.");
	}

	public static byte[] DecryptKey(byte[] keyData, RSA rsa, bool useOAEP)
	{
		if (keyData == null)
		{
			throw new ArgumentNullException("keyData");
		}
		if (rsa == null)
		{
			throw new ArgumentNullException("rsa");
		}
		if (useOAEP)
		{
			return new RSAOAEPKeyExchangeDeformatter(rsa).DecryptKeyExchange(keyData);
		}
		return new RSAPKCS1KeyExchangeDeformatter(rsa).DecryptKeyExchange(keyData);
	}
}

using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using System.Security.Permissions;
using Mono.Xml;

namespace System.Security.Cryptography;

[ComVisible(true)]
public class CryptoConfig
{
	private class CryptoHandler : SmallXmlParser.IContentHandler
	{
		private IDictionary<string, Type> algorithms;

		private IDictionary<string, string> oid;

		private Dictionary<string, string> names;

		private Dictionary<string, string> classnames;

		private int level;

		public CryptoHandler(IDictionary<string, Type> algorithms, IDictionary<string, string> oid)
		{
			this.algorithms = algorithms;
			this.oid = oid;
			names = new Dictionary<string, string>();
			classnames = new Dictionary<string, string>();
		}

		public void OnStartParsing(SmallXmlParser parser)
		{
		}

		public void OnEndParsing(SmallXmlParser parser)
		{
			foreach (KeyValuePair<string, string> name in names)
			{
				try
				{
					algorithms[name.Key] = Type.GetType(classnames[name.Value]);
				}
				catch
				{
				}
			}
			names.Clear();
			classnames.Clear();
		}

		private string Get(SmallXmlParser.IAttrList attrs, string name)
		{
			for (int i = 0; i < attrs.Names.Length; i++)
			{
				if (attrs.Names[i] == name)
				{
					return attrs.Values[i];
				}
			}
			return string.Empty;
		}

		public void OnStartElement(string name, SmallXmlParser.IAttrList attrs)
		{
			switch (level)
			{
			case 0:
				if (name == "configuration")
				{
					level++;
				}
				break;
			case 1:
				if (name == "mscorlib")
				{
					level++;
				}
				break;
			case 2:
				if (name == "cryptographySettings")
				{
					level++;
				}
				break;
			case 3:
				if (name == "oidMap")
				{
					level++;
				}
				else if (name == "cryptoNameMapping")
				{
					level++;
				}
				break;
			case 4:
				switch (name)
				{
				case "oidEntry":
					oid[Get(attrs, "name")] = Get(attrs, "OID");
					break;
				case "nameEntry":
					names[Get(attrs, "name")] = Get(attrs, "class");
					break;
				case "cryptoClasses":
					level++;
					break;
				}
				break;
			case 5:
				if (name == "cryptoClass")
				{
					classnames[attrs.Names[0]] = attrs.Values[0];
				}
				break;
			}
		}

		public void OnEndElement(string name)
		{
			switch (level)
			{
			case 1:
				if (name == "configuration")
				{
					level--;
				}
				break;
			case 2:
				if (name == "mscorlib")
				{
					level--;
				}
				break;
			case 3:
				if (name == "cryptographySettings")
				{
					level--;
				}
				break;
			case 4:
				if (name == "oidMap" || name == "cryptoNameMapping")
				{
					level--;
				}
				break;
			case 5:
				if (name == "cryptoClasses")
				{
					level--;
				}
				break;
			}
		}

		public void OnProcessingInstruction(string name, string text)
		{
		}

		public void OnChars(string text)
		{
		}

		public void OnIgnorableWhitespace(string text)
		{
		}
	}

	private static object lockObject;

	private static Dictionary<string, Type> algorithms;

	private static Dictionary<string, string> unresolved_algorithms;

	private static Dictionary<string, string> oids;

	private const string defaultNamespace = "System.Security.Cryptography.";

	private static Type defaultSHA1;

	private static Type defaultMD5;

	private static Type defaultSHA256;

	private static Type defaultSHA384;

	private static Type defaultSHA512;

	private static Type defaultRSA;

	private static Type defaultDSA;

	private static Type defaultDES;

	private static Type default3DES;

	private static Type defaultRC2;

	private static Type defaultAES;

	private static Type defaultRNG;

	private static Type defaultHMAC;

	private static Type defaultMAC3DES;

	private static Type defaultDSASigDesc;

	private static Type defaultRSAPKCS1SHA1SigDesc;

	private static Type defaultRSAPKCS1SHA256SigDesc;

	private static Type defaultRSAPKCS1SHA384SigDesc;

	private static Type defaultRSAPKCS1SHA512SigDesc;

	private static Type defaultRIPEMD160;

	private static Type defaultHMACMD5;

	private static Type defaultHMACRIPEMD160;

	private static Type defaultHMACSHA256;

	private static Type defaultHMACSHA384;

	private static Type defaultHMACSHA512;

	private const string defaultC14N = "System.Security.Cryptography.Xml.XmlDsigC14NTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultC14NWithComments = "System.Security.Cryptography.Xml.XmlDsigC14NWithCommentsTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultBase64 = "System.Security.Cryptography.Xml.XmlDsigBase64Transform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultXPath = "System.Security.Cryptography.Xml.XmlDsigXPathTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultXslt = "System.Security.Cryptography.Xml.XmlDsigXsltTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultEnveloped = "System.Security.Cryptography.Xml.XmlDsigEnvelopedSignatureTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultXmlDecryption = "System.Security.Cryptography.Xml.XmlDecryptionTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultExcC14N = "System.Security.Cryptography.Xml.XmlDsigExcC14NTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultExcC14NWithComments = "System.Security.Cryptography.Xml.XmlDsigExcC14NWithCommentsTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultX509Data = "System.Security.Cryptography.Xml.KeyInfoX509Data, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultKeyName = "System.Security.Cryptography.Xml.KeyInfoName, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultKeyValueDSA = "System.Security.Cryptography.Xml.DSAKeyValue, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultKeyValueRSA = "System.Security.Cryptography.Xml.RSAKeyValue, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string defaultRetrievalMethod = "System.Security.Cryptography.Xml.KeyInfoRetrievalMethod, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

	private const string managedSHA1 = "System.Security.Cryptography.SHA1Managed";

	private const string oidSHA1 = "1.3.14.3.2.26";

	private const string oidMD5 = "1.2.840.113549.2.5";

	private const string oidSHA256 = "2.16.840.1.101.3.4.2.1";

	private const string oidSHA384 = "2.16.840.1.101.3.4.2.2";

	private const string oidSHA512 = "2.16.840.1.101.3.4.2.3";

	private const string oidRIPEMD160 = "1.3.36.3.2.1";

	private const string oidDES = "1.3.14.3.2.7";

	private const string oid3DES = "1.2.840.113549.3.7";

	private const string oidRC2 = "1.2.840.113549.3.2";

	private const string oid3DESKeyWrap = "1.2.840.113549.1.9.16.3.6";

	private const string nameSHA1 = "System.Security.Cryptography.SHA1CryptoServiceProvider";

	private const string nameSHA1a = "SHA";

	private const string nameSHA1b = "SHA1";

	private const string nameSHA1c = "System.Security.Cryptography.SHA1";

	private const string nameSHA1d = "System.Security.Cryptography.HashAlgorithm";

	private const string nameMD5 = "System.Security.Cryptography.MD5CryptoServiceProvider";

	private const string nameMD5a = "MD5";

	private const string nameMD5b = "System.Security.Cryptography.MD5";

	private const string nameSHA256 = "System.Security.Cryptography.SHA256Managed";

	private const string nameSHA256a = "SHA256";

	private const string nameSHA256b = "SHA-256";

	private const string nameSHA256c = "System.Security.Cryptography.SHA256";

	private const string nameSHA384 = "System.Security.Cryptography.SHA384Managed";

	private const string nameSHA384a = "SHA384";

	private const string nameSHA384b = "SHA-384";

	private const string nameSHA384c = "System.Security.Cryptography.SHA384";

	private const string nameSHA512 = "System.Security.Cryptography.SHA512Managed";

	private const string nameSHA512a = "SHA512";

	private const string nameSHA512b = "SHA-512";

	private const string nameSHA512c = "System.Security.Cryptography.SHA512";

	private const string nameRSAa = "RSA";

	private const string nameRSAb = "System.Security.Cryptography.RSA";

	private const string nameRSAc = "System.Security.Cryptography.AsymmetricAlgorithm";

	private const string nameDSAa = "DSA";

	private const string nameDSAb = "System.Security.Cryptography.DSA";

	private const string nameDESa = "DES";

	private const string nameDESb = "System.Security.Cryptography.DES";

	private const string name3DESa = "3DES";

	private const string name3DESb = "TripleDES";

	private const string name3DESc = "Triple DES";

	private const string name3DESd = "System.Security.Cryptography.TripleDES";

	private const string nameRC2a = "RC2";

	private const string nameRC2b = "System.Security.Cryptography.RC2";

	private const string nameAESa = "Rijndael";

	private const string nameAESb = "System.Security.Cryptography.Rijndael";

	private const string nameAESc = "System.Security.Cryptography.SymmetricAlgorithm";

	private const string nameRNGa = "RandomNumberGenerator";

	private const string nameRNGb = "System.Security.Cryptography.RandomNumberGenerator";

	private const string nameKeyHasha = "System.Security.Cryptography.KeyedHashAlgorithm";

	private const string nameHMACSHA1a = "HMACSHA1";

	private const string nameHMACSHA1b = "System.Security.Cryptography.HMACSHA1";

	private const string nameMAC3DESa = "MACTripleDES";

	private const string nameMAC3DESb = "System.Security.Cryptography.MACTripleDES";

	private const string name3DESKeyWrap = "TripleDESKeyWrap";

	private const string nameRIPEMD160 = "System.Security.Cryptography.RIPEMD160Managed";

	private const string nameRIPEMD160a = "RIPEMD160";

	private const string nameRIPEMD160b = "RIPEMD-160";

	private const string nameRIPEMD160c = "System.Security.Cryptography.RIPEMD160";

	private const string nameHMACb = "System.Security.Cryptography.HMAC";

	private const string nameHMACMD5a = "HMACMD5";

	private const string nameHMACMD5b = "System.Security.Cryptography.HMACMD5";

	private const string nameHMACRIPEMD160a = "HMACRIPEMD160";

	private const string nameHMACRIPEMD160b = "System.Security.Cryptography.HMACRIPEMD160";

	private const string nameHMACSHA256a = "HMACSHA256";

	private const string nameHMACSHA256b = "System.Security.Cryptography.HMACSHA256";

	private const string nameHMACSHA384a = "HMACSHA384";

	private const string nameHMACSHA384b = "System.Security.Cryptography.HMACSHA384";

	private const string nameHMACSHA512a = "HMACSHA512";

	private const string nameHMACSHA512b = "System.Security.Cryptography.HMACSHA512";

	private const string urlXmlDsig = "http://www.w3.org/2000/09/xmldsig#";

	private const string urlDSASHA1 = "http://www.w3.org/2000/09/xmldsig#dsa-sha1";

	private const string urlRSASHA1 = "http://www.w3.org/2000/09/xmldsig#rsa-sha1";

	private const string urlRSASHA256 = "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256";

	private const string urlRSASHA384 = "http://www.w3.org/2001/04/xmldsig-more#rsa-sha384";

	private const string urlRSASHA512 = "http://www.w3.org/2001/04/xmldsig-more#rsa-sha512";

	private const string urlSHA1 = "http://www.w3.org/2000/09/xmldsig#sha1";

	private const string urlC14N = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";

	private const string urlC14NWithComments = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments";

	private const string urlBase64 = "http://www.w3.org/2000/09/xmldsig#base64";

	private const string urlXPath = "http://www.w3.org/TR/1999/REC-xpath-19991116";

	private const string urlXslt = "http://www.w3.org/TR/1999/REC-xslt-19991116";

	private const string urlEnveloped = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";

	private const string urlXmlDecryption = "http://www.w3.org/2002/07/decrypt#XML";

	private const string urlExcC14NWithComments = "http://www.w3.org/2001/10/xml-exc-c14n#WithComments";

	private const string urlExcC14N = "http://www.w3.org/2001/10/xml-exc-c14n#";

	private const string urlSHA256 = "http://www.w3.org/2001/04/xmlenc#sha256";

	private const string urlSHA384 = "http://www.w3.org/2001/04/xmldsig-more#sha384";

	private const string urlSHA512 = "http://www.w3.org/2001/04/xmlenc#sha512";

	private const string urlHMACSHA256 = "http://www.w3.org/2001/04/xmldsig-more#hmac-sha256";

	private const string urlHMACSHA384 = "http://www.w3.org/2001/04/xmldsig-more#hmac-sha384";

	private const string urlHMACSHA512 = "http://www.w3.org/2001/04/xmldsig-more#hmac-sha512";

	private const string urlHMACRIPEMD160 = "http://www.w3.org/2001/04/xmldsig-more#hmac-ripemd160";

	private const string urlX509Data = "http://www.w3.org/2000/09/xmldsig# X509Data";

	private const string urlKeyName = "http://www.w3.org/2000/09/xmldsig# KeyName";

	private const string urlKeyValueDSA = "http://www.w3.org/2000/09/xmldsig# KeyValue/DSAKeyValue";

	private const string urlKeyValueRSA = "http://www.w3.org/2000/09/xmldsig# KeyValue/RSAKeyValue";

	private const string urlRetrievalMethod = "http://www.w3.org/2000/09/xmldsig# RetrievalMethod";

	private const string oidX509SubjectKeyIdentifier = "2.5.29.14";

	private const string oidX509KeyUsage = "2.5.29.15";

	private const string oidX509BasicConstraints = "2.5.29.19";

	private const string oidX509EnhancedKeyUsage = "2.5.29.37";

	private const string nameX509SubjectKeyIdentifier = "System.Security.Cryptography.X509Certificates.X509SubjectKeyIdentifierExtension, System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameX509KeyUsage = "System.Security.Cryptography.X509Certificates.X509KeyUsageExtension, System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameX509BasicConstraints = "System.Security.Cryptography.X509Certificates.X509BasicConstraintsExtension, System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameX509EnhancedKeyUsage = "System.Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension, System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameX509Chain = "X509Chain";

	private const string defaultX509Chain = "System.Security.Cryptography.X509Certificates.X509Chain, System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string system_core_assembly = ", System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameAES_1 = "AES";

	private const string nameAES_2 = "System.Security.Cryptography.AesCryptoServiceProvider";

	private const string defaultAES_1 = "System.Security.Cryptography.AesCryptoServiceProvider, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameAESManaged_1 = "AesManaged";

	private const string nameAESManaged_2 = "System.Security.Cryptography.AesManaged";

	private const string defaultAESManaged = "System.Security.Cryptography.AesManaged, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameECDiffieHellman_1 = "ECDH";

	private const string nameECDiffieHellman_2 = "ECDiffieHellman";

	private const string nameECDiffieHellman_3 = "ECDiffieHellmanCng";

	private const string nameECDiffieHellman_4 = "System.Security.Cryptography.ECDiffieHellmanCng";

	private const string defaultECDiffieHellman = "System.Security.Cryptography.ECDiffieHellmanCng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameECDsa_1 = "ECDsa";

	private const string nameECDsa_2 = "ECDsaCng";

	private const string nameECDsa_3 = "System.Security.Cryptography.ECDsaCng";

	private const string defaultECDsa = "System.Security.Cryptography.ECDsaCng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameSHA1Cng = "System.Security.Cryptography.SHA1Cng";

	private const string defaultSHA1Cng = "System.Security.Cryptography.SHA1Cng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameSHA256Cng = "System.Security.Cryptography.SHA256Cng";

	private const string defaultSHA256Cng = "System.Security.Cryptography.SHA256Cng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameSHA256Provider = "System.Security.Cryptography.SHA256CryptoServiceProvider";

	private const string defaultSHA256Provider = "System.Security.Cryptography.SHA256CryptoServiceProvider, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameSHA384Cng = "System.Security.Cryptography.SHA384Cng";

	private const string defaultSHA384Cng = "System.Security.Cryptography.SHA384Cng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameSHA384Provider = "System.Security.Cryptography.SHA384CryptoServiceProvider";

	private const string defaultSHA384Provider = "System.Security.Cryptography.SHA384CryptoServiceProvider, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameSHA512Cng = "System.Security.Cryptography.SHA512Cng";

	private const string defaultSHA512Cng = "System.Security.Cryptography.SHA512Cng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	private const string nameSHA512Provider = "System.Security.Cryptography.SHA512CryptoServiceProvider";

	private const string defaultSHA512Provider = "System.Security.Cryptography.SHA512CryptoServiceProvider, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";

	[MonoLimitation("nothing is FIPS certified so it never make sense to restrict to this (empty) subset")]
	public static bool AllowOnlyFipsAlgorithms => false;

	public static byte[] EncodeOID(string str)
	{
		if (str == null)
		{
			throw new ArgumentNullException("str");
		}
		char[] separator = new char[1] { '.' };
		string[] array = str.Split(separator);
		if (array.Length < 2)
		{
			throw new CryptographicUnexpectedOperationException(Locale.GetText("OID must have at least two parts"));
		}
		byte[] array2 = new byte[str.Length];
		try
		{
			byte b = Convert.ToByte(array[0]);
			byte b2 = Convert.ToByte(array[1]);
			array2[2] = Convert.ToByte(b * 40 + b2);
		}
		catch
		{
			throw new CryptographicUnexpectedOperationException(Locale.GetText("Invalid OID"));
		}
		int num = 3;
		for (int i = 2; i < array.Length; i++)
		{
			long num2 = Convert.ToInt64(array[i]);
			if (num2 > 127)
			{
				byte[] array3 = EncodeLongNumber(num2);
				Buffer.BlockCopy(array3, 0, array2, num, array3.Length);
				num += array3.Length;
			}
			else
			{
				array2[num++] = Convert.ToByte(num2);
			}
		}
		int num3 = 2;
		byte[] array4 = new byte[num];
		array4[0] = 6;
		if (num > 127)
		{
			throw new CryptographicUnexpectedOperationException(Locale.GetText("OID > 127 bytes"));
		}
		array4[1] = Convert.ToByte(num - 2);
		Buffer.BlockCopy(array2, num3, array4, num3, num - num3);
		return array4;
	}

	private static byte[] EncodeLongNumber(long x)
	{
		if (x > int.MaxValue || x < int.MinValue)
		{
			throw new OverflowException(Locale.GetText("Part of OID doesn't fit in Int32"));
		}
		long num = x;
		int num2 = 1;
		while (num > 127)
		{
			num >>= 7;
			num2++;
		}
		byte[] array = new byte[num2];
		for (int i = 0; i < num2; i++)
		{
			num = x >> 7 * i;
			num &= 0x7F;
			if (i != 0)
			{
				num += 128;
			}
			array[num2 - i - 1] = Convert.ToByte(num);
		}
		return array;
	}

	static CryptoConfig()
	{
		defaultSHA1 = typeof(SHA1CryptoServiceProvider);
		defaultMD5 = typeof(MD5CryptoServiceProvider);
		defaultSHA256 = typeof(SHA256Managed);
		defaultSHA384 = typeof(SHA384Managed);
		defaultSHA512 = typeof(SHA512Managed);
		defaultRSA = typeof(RSACryptoServiceProvider);
		defaultDSA = typeof(DSACryptoServiceProvider);
		defaultDES = typeof(DESCryptoServiceProvider);
		default3DES = typeof(TripleDESCryptoServiceProvider);
		defaultRC2 = typeof(RC2CryptoServiceProvider);
		defaultAES = typeof(RijndaelManaged);
		defaultRNG = typeof(RNGCryptoServiceProvider);
		defaultHMAC = typeof(HMACSHA1);
		defaultMAC3DES = typeof(MACTripleDES);
		defaultDSASigDesc = typeof(DSASignatureDescription);
		defaultRSAPKCS1SHA1SigDesc = typeof(RSAPKCS1SHA1SignatureDescription);
		defaultRSAPKCS1SHA256SigDesc = typeof(RSAPKCS1SHA256SignatureDescription);
		defaultRSAPKCS1SHA384SigDesc = typeof(RSAPKCS1SHA384SignatureDescription);
		defaultRSAPKCS1SHA512SigDesc = typeof(RSAPKCS1SHA512SignatureDescription);
		defaultRIPEMD160 = typeof(RIPEMD160Managed);
		defaultHMACMD5 = typeof(HMACMD5);
		defaultHMACRIPEMD160 = typeof(HMACRIPEMD160);
		defaultHMACSHA256 = typeof(HMACSHA256);
		defaultHMACSHA384 = typeof(HMACSHA384);
		defaultHMACSHA512 = typeof(HMACSHA512);
		lockObject = new object();
	}

	private static void Initialize()
	{
		Dictionary<string, Type> dictionary = new Dictionary<string, Type>(StringComparer.OrdinalIgnoreCase);
		dictionary.Add("SHA", defaultSHA1);
		dictionary.Add("SHA1", defaultSHA1);
		dictionary.Add("System.Security.Cryptography.SHA1", defaultSHA1);
		dictionary.Add("System.Security.Cryptography.HashAlgorithm", defaultSHA1);
		dictionary.Add("MD5", defaultMD5);
		dictionary.Add("System.Security.Cryptography.MD5", defaultMD5);
		dictionary.Add("SHA256", defaultSHA256);
		dictionary.Add("SHA-256", defaultSHA256);
		dictionary.Add("System.Security.Cryptography.SHA256", defaultSHA256);
		dictionary.Add("SHA384", defaultSHA384);
		dictionary.Add("SHA-384", defaultSHA384);
		dictionary.Add("System.Security.Cryptography.SHA384", defaultSHA384);
		dictionary.Add("SHA512", defaultSHA512);
		dictionary.Add("SHA-512", defaultSHA512);
		dictionary.Add("System.Security.Cryptography.SHA512", defaultSHA512);
		dictionary.Add("RSA", defaultRSA);
		dictionary.Add("System.Security.Cryptography.RSA", defaultRSA);
		dictionary.Add("System.Security.Cryptography.AsymmetricAlgorithm", defaultRSA);
		dictionary.Add("DSA", defaultDSA);
		dictionary.Add("System.Security.Cryptography.DSA", defaultDSA);
		dictionary.Add("DES", defaultDES);
		dictionary.Add("System.Security.Cryptography.DES", defaultDES);
		dictionary.Add("3DES", default3DES);
		dictionary.Add("TripleDES", default3DES);
		dictionary.Add("Triple DES", default3DES);
		dictionary.Add("System.Security.Cryptography.TripleDES", default3DES);
		dictionary.Add("RC2", defaultRC2);
		dictionary.Add("System.Security.Cryptography.RC2", defaultRC2);
		dictionary.Add("Rijndael", defaultAES);
		dictionary.Add("System.Security.Cryptography.Rijndael", defaultAES);
		dictionary.Add("System.Security.Cryptography.SymmetricAlgorithm", defaultAES);
		dictionary.Add("RandomNumberGenerator", defaultRNG);
		dictionary.Add("System.Security.Cryptography.RandomNumberGenerator", defaultRNG);
		dictionary.Add("System.Security.Cryptography.KeyedHashAlgorithm", defaultHMAC);
		dictionary.Add("HMACSHA1", defaultHMAC);
		dictionary.Add("System.Security.Cryptography.HMACSHA1", defaultHMAC);
		dictionary.Add("MACTripleDES", defaultMAC3DES);
		dictionary.Add("System.Security.Cryptography.MACTripleDES", defaultMAC3DES);
		dictionary.Add("RIPEMD160", defaultRIPEMD160);
		dictionary.Add("RIPEMD-160", defaultRIPEMD160);
		dictionary.Add("System.Security.Cryptography.RIPEMD160", defaultRIPEMD160);
		dictionary.Add("System.Security.Cryptography.HMAC", defaultHMAC);
		dictionary.Add("HMACMD5", defaultHMACMD5);
		dictionary.Add("System.Security.Cryptography.HMACMD5", defaultHMACMD5);
		dictionary.Add("HMACRIPEMD160", defaultHMACRIPEMD160);
		dictionary.Add("System.Security.Cryptography.HMACRIPEMD160", defaultHMACRIPEMD160);
		dictionary.Add("HMACSHA256", defaultHMACSHA256);
		dictionary.Add("System.Security.Cryptography.HMACSHA256", defaultHMACSHA256);
		dictionary.Add("HMACSHA384", defaultHMACSHA384);
		dictionary.Add("System.Security.Cryptography.HMACSHA384", defaultHMACSHA384);
		dictionary.Add("HMACSHA512", defaultHMACSHA512);
		dictionary.Add("System.Security.Cryptography.HMACSHA512", defaultHMACSHA512);
		Dictionary<string, string> dictionary2 = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
		dictionary.Add("http://www.w3.org/2000/09/xmldsig#dsa-sha1", defaultDSASigDesc);
		dictionary.Add("http://www.w3.org/2000/09/xmldsig#rsa-sha1", defaultRSAPKCS1SHA1SigDesc);
		dictionary.Add("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256", defaultRSAPKCS1SHA256SigDesc);
		dictionary.Add("http://www.w3.org/2001/04/xmldsig-more#rsa-sha384", defaultRSAPKCS1SHA384SigDesc);
		dictionary.Add("http://www.w3.org/2001/04/xmldsig-more#rsa-sha512", defaultRSAPKCS1SHA512SigDesc);
		dictionary.Add("http://www.w3.org/2000/09/xmldsig#sha1", defaultSHA1);
		dictionary2.Add("http://www.w3.org/TR/2001/REC-xml-c14n-20010315", "System.Security.Cryptography.Xml.XmlDsigC14NTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments", "System.Security.Cryptography.Xml.XmlDsigC14NWithCommentsTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/2000/09/xmldsig#base64", "System.Security.Cryptography.Xml.XmlDsigBase64Transform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/TR/1999/REC-xpath-19991116", "System.Security.Cryptography.Xml.XmlDsigXPathTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/TR/1999/REC-xslt-19991116", "System.Security.Cryptography.Xml.XmlDsigXsltTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/2000/09/xmldsig#enveloped-signature", "System.Security.Cryptography.Xml.XmlDsigEnvelopedSignatureTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/2001/10/xml-exc-c14n#", "System.Security.Cryptography.Xml.XmlDsigExcC14NTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/2001/10/xml-exc-c14n#WithComments", "System.Security.Cryptography.Xml.XmlDsigExcC14NWithCommentsTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/2002/07/decrypt#XML", "System.Security.Cryptography.Xml.XmlDecryptionTransform, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary.Add("http://www.w3.org/2001/04/xmlenc#sha256", defaultSHA256);
		dictionary.Add("http://www.w3.org/2001/04/xmldsig-more#sha384", defaultSHA384);
		dictionary.Add("http://www.w3.org/2001/04/xmlenc#sha512", defaultSHA512);
		dictionary.Add("http://www.w3.org/2001/04/xmldsig-more#hmac-sha256", defaultHMACSHA256);
		dictionary.Add("http://www.w3.org/2001/04/xmldsig-more#hmac-sha384", defaultHMACSHA384);
		dictionary.Add("http://www.w3.org/2001/04/xmldsig-more#hmac-sha512", defaultHMACSHA512);
		dictionary.Add("http://www.w3.org/2001/04/xmldsig-more#hmac-ripemd160", defaultHMACRIPEMD160);
		dictionary2.Add("http://www.w3.org/2000/09/xmldsig# X509Data", "System.Security.Cryptography.Xml.KeyInfoX509Data, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/2000/09/xmldsig# KeyName", "System.Security.Cryptography.Xml.KeyInfoName, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/2000/09/xmldsig# KeyValue/DSAKeyValue", "System.Security.Cryptography.Xml.DSAKeyValue, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/2000/09/xmldsig# KeyValue/RSAKeyValue", "System.Security.Cryptography.Xml.RSAKeyValue, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("http://www.w3.org/2000/09/xmldsig# RetrievalMethod", "System.Security.Cryptography.Xml.KeyInfoRetrievalMethod, System.Security, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a");
		dictionary2.Add("2.5.29.14", "System.Security.Cryptography.X509Certificates.X509SubjectKeyIdentifierExtension, System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("2.5.29.15", "System.Security.Cryptography.X509Certificates.X509KeyUsageExtension, System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("2.5.29.19", "System.Security.Cryptography.X509Certificates.X509BasicConstraintsExtension, System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("2.5.29.37", "System.Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension, System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("X509Chain", "System.Security.Cryptography.X509Certificates.X509Chain, System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("AES", "System.Security.Cryptography.AesCryptoServiceProvider, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.AesCryptoServiceProvider", "System.Security.Cryptography.AesCryptoServiceProvider, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("AesManaged", "System.Security.Cryptography.AesManaged, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.AesManaged", "System.Security.Cryptography.AesManaged, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("ECDH", "System.Security.Cryptography.ECDiffieHellmanCng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("ECDiffieHellman", "System.Security.Cryptography.ECDiffieHellmanCng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("ECDiffieHellmanCng", "System.Security.Cryptography.ECDiffieHellmanCng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.ECDiffieHellmanCng", "System.Security.Cryptography.ECDiffieHellmanCng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("ECDsa", "System.Security.Cryptography.ECDsaCng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("ECDsaCng", "System.Security.Cryptography.ECDsaCng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.ECDsaCng", "System.Security.Cryptography.ECDsaCng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.SHA1Cng", "System.Security.Cryptography.SHA1Cng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.SHA256Cng", "System.Security.Cryptography.SHA256Cng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.SHA256CryptoServiceProvider", "System.Security.Cryptography.SHA256CryptoServiceProvider, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.SHA384Cng", "System.Security.Cryptography.SHA384Cng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.SHA384CryptoServiceProvider", "System.Security.Cryptography.SHA384CryptoServiceProvider, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.SHA512Cng", "System.Security.Cryptography.SHA512Cng, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		dictionary2.Add("System.Security.Cryptography.SHA512CryptoServiceProvider", "System.Security.Cryptography.SHA512CryptoServiceProvider, System.Core, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
		Dictionary<string, string> oid = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
		{
			{ "System.Security.Cryptography.SHA1CryptoServiceProvider", "1.3.14.3.2.26" },
			{ "System.Security.Cryptography.SHA1Managed", "1.3.14.3.2.26" },
			{ "SHA1", "1.3.14.3.2.26" },
			{ "System.Security.Cryptography.SHA1", "1.3.14.3.2.26" },
			{ "System.Security.Cryptography.SHA1Cng", "1.3.14.3.2.26" },
			{ "System.Security.Cryptography.MD5CryptoServiceProvider", "1.2.840.113549.2.5" },
			{ "MD5", "1.2.840.113549.2.5" },
			{ "System.Security.Cryptography.MD5", "1.2.840.113549.2.5" },
			{ "System.Security.Cryptography.SHA256Managed", "2.16.840.1.101.3.4.2.1" },
			{ "SHA256", "2.16.840.1.101.3.4.2.1" },
			{ "System.Security.Cryptography.SHA256", "2.16.840.1.101.3.4.2.1" },
			{ "System.Security.Cryptography.SHA256Cng", "2.16.840.1.101.3.4.2.1" },
			{ "System.Security.Cryptography.SHA256CryptoServiceProvider", "2.16.840.1.101.3.4.2.1" },
			{ "System.Security.Cryptography.SHA384Managed", "2.16.840.1.101.3.4.2.2" },
			{ "SHA384", "2.16.840.1.101.3.4.2.2" },
			{ "System.Security.Cryptography.SHA384", "2.16.840.1.101.3.4.2.2" },
			{ "System.Security.Cryptography.SHA384Cng", "2.16.840.1.101.3.4.2.2" },
			{ "System.Security.Cryptography.SHA384CryptoServiceProvider", "2.16.840.1.101.3.4.2.2" },
			{ "System.Security.Cryptography.SHA512Managed", "2.16.840.1.101.3.4.2.3" },
			{ "SHA512", "2.16.840.1.101.3.4.2.3" },
			{ "System.Security.Cryptography.SHA512", "2.16.840.1.101.3.4.2.3" },
			{ "System.Security.Cryptography.SHA512Cng", "2.16.840.1.101.3.4.2.3" },
			{ "System.Security.Cryptography.SHA512CryptoServiceProvider", "2.16.840.1.101.3.4.2.3" },
			{ "System.Security.Cryptography.RIPEMD160Managed", "1.3.36.3.2.1" },
			{ "RIPEMD160", "1.3.36.3.2.1" },
			{ "System.Security.Cryptography.RIPEMD160", "1.3.36.3.2.1" },
			{ "TripleDESKeyWrap", "1.2.840.113549.1.9.16.3.6" },
			{ "DES", "1.3.14.3.2.7" },
			{ "TripleDES", "1.2.840.113549.3.7" },
			{ "RC2", "1.2.840.113549.3.2" }
		};
		LoadConfig(Environment.GetMachineConfigPath(), dictionary, oid);
		algorithms = dictionary;
		unresolved_algorithms = dictionary2;
		oids = oid;
	}

	[FileIOPermission(SecurityAction.Assert, Unrestricted = true)]
	private static void LoadConfig(string filename, IDictionary<string, Type> algorithms, IDictionary<string, string> oid)
	{
		if (!File.Exists(filename))
		{
			return;
		}
		try
		{
			using TextReader input = new StreamReader(filename);
			CryptoHandler handler = new CryptoHandler(algorithms, oid);
			new SmallXmlParser().Parse(input, handler);
		}
		catch
		{
		}
	}

	public static object CreateFromName(string name)
	{
		return CreateFromName(name, null);
	}

	[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
	public static object CreateFromName(string name, params object[] args)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		lock (lockObject)
		{
			if (algorithms == null)
			{
				Initialize();
			}
		}
		try
		{
			Type value = null;
			if (!algorithms.TryGetValue(name, out value))
			{
				string value2 = null;
				if (!unresolved_algorithms.TryGetValue(name, out value2))
				{
					value2 = name;
				}
				value = Type.GetType(value2);
			}
			if (value == null)
			{
				return null;
			}
			return Activator.CreateInstance(value, args);
		}
		catch
		{
			return null;
		}
	}

	internal static string MapNameToOID(string name, OidGroup oidGroup)
	{
		return MapNameToOID(name);
	}

	public static string MapNameToOID(string name)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		lock (lockObject)
		{
			if (oids == null)
			{
				Initialize();
			}
		}
		string value = null;
		oids.TryGetValue(name, out value);
		return value;
	}

	public static void AddAlgorithm(Type algorithm, params string[] names)
	{
		if (algorithm == null)
		{
			throw new ArgumentNullException("algorithm");
		}
		if (names == null)
		{
			throw new ArgumentNullException("names");
		}
		foreach (string text in names)
		{
			if (string.IsNullOrWhiteSpace(text))
			{
				throw new ArithmeticException("names");
			}
			algorithms[text] = algorithm;
		}
	}

	public static void AddOID(string oid, params string[] names)
	{
		if (oid == null)
		{
			throw new ArgumentNullException("oid");
		}
		if (names == null)
		{
			throw new ArgumentNullException("names");
		}
		foreach (string value in names)
		{
			if (string.IsNullOrWhiteSpace(value))
			{
				throw new ArithmeticException("names");
			}
			oids[oid] = value;
		}
	}
}

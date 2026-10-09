namespace System.Security.Cryptography;

internal static class CAPI
{
	internal const uint CRYPT_OID_INFO_OID_KEY = 1u;

	internal const uint CRYPT_OID_INFO_NAME_KEY = 2u;

	internal const uint CRYPT_OID_INFO_ALGID_KEY = 3u;

	internal const uint CRYPT_OID_INFO_SIGN_KEY = 4u;

	public static string CryptFindOIDInfoNameFromKey(string key, OidGroup oidGroup)
	{
		switch (key)
		{
		case "1.2.840.113549.1.1.5":
		case "1.3.14.3.2.29":
		case "1.3.14.3.2.15":
			return "sha1RSA";
		case "1.2.840.113549.1.1.4":
		case "1.3.14.3.2.3":
			return "md5RSA";
		case "1.2.840.10040.4.3":
		case "1.3.14.3.2.13":
			return "sha1DSA";
		case "1.2.840.113549.1.1.2":
		case "1.3.14.7.2.3.1":
			return "md2RSA";
		case "1.2.840.113549.1.1.3":
			return "md4RSA";
		case "1.3.14.3.2.27":
			return "dsaSHA1";
		case "2.16.840.1.101.2.1.1.19":
			return "mosaicUpdatedSig";
		case "1.3.14.3.2.26":
			return "sha1";
		case "1.2.840.113549.2.5":
			return "md5";
		case "2.16.840.1.101.3.4.2.1":
			return "sha256";
		case "2.16.840.1.101.3.4.2.2":
			return "sha384";
		case "2.16.840.1.101.3.4.2.3":
			return "sha512";
		case "1.2.840.113549.1.1.11":
			return "sha256RSA";
		case "1.2.840.113549.1.1.12":
			return "sha384RSA";
		case "1.2.840.113549.1.1.13":
			return "sha512RSA";
		case "1.2.840.113549.1.1.10":
			return "RSASSA-PSS";
		case "1.2.840.10045.4.1":
			return "sha1ECDSA";
		case "1.2.840.10045.4.3.2":
			return "sha256ECDSA";
		case "1.2.840.10045.4.3.3":
			return "sha384ECDSA";
		case "1.2.840.10045.4.3.4":
			return "sha512ECDSA";
		case "1.2.840.10045.4.3":
			return "specifiedECDSA";
		case "1.2.840.113549.1.1.1":
			return "RSA";
		case "1.2.840.113549.1.7.1":
			return "PKCS 7 Data";
		case "1.2.840.113549.1.9.3":
			return "Content Type";
		case "1.2.840.113549.1.9.4":
			return "Message Digest";
		case "1.2.840.113549.1.9.5":
			return "Signing Time";
		case "1.2.840.113549.3.7":
			return "3des";
		case "2.5.29.17":
			return "Subject Alternative Name";
		case "2.16.840.1.101.3.4.1.2":
			return "aes128";
		case "2.16.840.1.101.3.4.1.42":
			return "aes256";
		case "2.16.840.1.113730.1.1":
			return "Netscape Cert Type";
		default:
			return null;
		}
	}

	public static string CryptFindOIDInfoKeyFromName(string name, OidGroup oidGroup)
	{
		return name switch
		{
			"sha1RSA" => "1.2.840.113549.1.1.5", 
			"md5RSA" => "1.2.840.113549.1.1.4", 
			"sha1DSA" => "1.2.840.10040.4.3", 
			"shaRSA" => "1.3.14.3.2.29", 
			"md2RSA" => "1.2.840.113549.1.1.2", 
			"md4RSA" => "1.2.840.113549.1.1.3", 
			"dsaSHA1" => "1.3.14.3.2.27", 
			"mosaicUpdatedSig" => "2.16.840.1.101.2.1.1.19", 
			"sha1" => "1.3.14.3.2.26", 
			"md5" => "1.2.840.113549.2.5", 
			"sha256" => "2.16.840.1.101.3.4.2.1", 
			"sha384" => "2.16.840.1.101.3.4.2.2", 
			"sha512" => "2.16.840.1.101.3.4.2.3", 
			"sha256RSA" => "1.2.840.113549.1.1.11", 
			"sha384RSA" => "1.2.840.113549.1.1.12", 
			"sha512RSA" => "1.2.840.113549.1.1.13", 
			"RSASSA-PSS" => "1.2.840.113549.1.1.10", 
			"sha1ECDSA" => "1.2.840.10045.4.1", 
			"sha256ECDSA" => "1.2.840.10045.4.3.2", 
			"sha384ECDSA" => "1.2.840.10045.4.3.3", 
			"sha512ECDSA" => "1.2.840.10045.4.3.4", 
			"specifiedECDSA" => "1.2.840.10045.4.3", 
			"RSA" => "1.2.840.113549.1.1.1", 
			"PKCS 7 Data" => "1.2.840.113549.1.7.1", 
			"Content Type" => "1.2.840.113549.1.9.3", 
			"Message Digest" => "1.2.840.113549.1.9.4", 
			"Signing Time" => "1.2.840.113549.1.9.5", 
			"3des" => "1.2.840.113549.3.7", 
			"Subject Alternative Name" => "2.5.29.17", 
			"aes128" => "2.16.840.1.101.3.4.1.2", 
			"aes256" => "2.16.840.1.101.3.4.1.42", 
			"Netscape Cert Type" => "2.16.840.1.113730.1.1", 
			_ => null, 
		};
	}
}

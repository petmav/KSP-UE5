namespace System.Security.Cryptography.X509Certificates;

internal class X509Utils
{
	private X509Utils()
	{
	}

	internal static string FindOidInfo(uint keyType, string keyValue, System.Security.Cryptography.OidGroup oidGroup)
	{
		if (keyValue == null)
		{
			throw new ArgumentNullException("keyValue");
		}
		if (keyValue.Length == 0)
		{
			return null;
		}
		return keyType switch
		{
			1u => CAPI.CryptFindOIDInfoNameFromKey(keyValue, oidGroup), 
			2u => CAPI.CryptFindOIDInfoKeyFromName(keyValue, oidGroup), 
			_ => throw new NotImplementedException(keyType.ToString()), 
		};
	}

	internal static string FindOidInfoWithFallback(uint key, string value, System.Security.Cryptography.OidGroup group)
	{
		string text = FindOidInfo(key, value, group);
		if (text == null && group != 0)
		{
			text = FindOidInfo(key, value, System.Security.Cryptography.OidGroup.All);
		}
		return text;
	}
}

using Mono.Security.Cryptography;

namespace System.Security.Cryptography;

public sealed class ProtectedData
{
	private enum DataProtectionImplementation
	{
		Unknown = 0,
		Win32CryptoProtect = 1,
		ManagedProtection = 2,
		Unsupported = int.MinValue
	}

	private static DataProtectionImplementation impl;

	private ProtectedData()
	{
	}

	public static byte[] Protect(byte[] userData, byte[] optionalEntropy, DataProtectionScope scope)
	{
		if (userData == null)
		{
			throw new ArgumentNullException("userData");
		}
		Check(scope);
		switch (impl)
		{
		case DataProtectionImplementation.ManagedProtection:
			try
			{
				return ManagedProtection.Protect(userData, optionalEntropy, scope);
			}
			catch (Exception inner2)
			{
				throw new CryptographicException(global::Locale.GetText("Data protection failed."), inner2);
			}
		case DataProtectionImplementation.Win32CryptoProtect:
			try
			{
				return NativeDapiProtection.Protect(userData, optionalEntropy, scope);
			}
			catch (Exception inner)
			{
				throw new CryptographicException(global::Locale.GetText("Data protection failed."), inner);
			}
		default:
			throw new PlatformNotSupportedException();
		}
	}

	public static byte[] Unprotect(byte[] encryptedData, byte[] optionalEntropy, DataProtectionScope scope)
	{
		if (encryptedData == null)
		{
			throw new ArgumentNullException("encryptedData");
		}
		Check(scope);
		switch (impl)
		{
		case DataProtectionImplementation.ManagedProtection:
			try
			{
				return ManagedProtection.Unprotect(encryptedData, optionalEntropy, scope);
			}
			catch (Exception inner2)
			{
				throw new CryptographicException(global::Locale.GetText("Data unprotection failed."), inner2);
			}
		case DataProtectionImplementation.Win32CryptoProtect:
			try
			{
				return NativeDapiProtection.Unprotect(encryptedData, optionalEntropy, scope);
			}
			catch (Exception inner)
			{
				throw new CryptographicException(global::Locale.GetText("Data unprotection failed."), inner);
			}
		default:
			throw new PlatformNotSupportedException();
		}
	}

	private static void Detect()
	{
		OperatingSystem oSVersion = Environment.OSVersion;
		switch (oSVersion.Platform)
		{
		case PlatformID.Win32NT:
			if (oSVersion.Version.Major < 5)
			{
				impl = DataProtectionImplementation.Unsupported;
			}
			else
			{
				impl = DataProtectionImplementation.Win32CryptoProtect;
			}
			break;
		case PlatformID.Unix:
			impl = DataProtectionImplementation.ManagedProtection;
			break;
		default:
			impl = DataProtectionImplementation.Unsupported;
			break;
		}
	}

	private static void Check(DataProtectionScope scope)
	{
		if (scope < DataProtectionScope.CurrentUser || scope > DataProtectionScope.LocalMachine)
		{
			throw new ArgumentException(global::Locale.GetText("Invalid enum value '{0}' for '{1}'.", scope, "DataProtectionScope"), "scope");
		}
		switch (impl)
		{
		case DataProtectionImplementation.Unknown:
			Detect();
			break;
		case DataProtectionImplementation.Unsupported:
			throw new PlatformNotSupportedException();
		}
	}
}

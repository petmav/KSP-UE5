using System.Runtime.InteropServices;

namespace System.Security.Cryptography;

public sealed class ProtectedMemory
{
	private enum MemoryProtectionImplementation
	{
		Unknown = 0,
		Win32RtlEncryptMemory = 1,
		Win32CryptoProtect = 2,
		Unsupported = int.MinValue
	}

	private const int BlockSize = 16;

	private static MemoryProtectionImplementation impl;

	private ProtectedMemory()
	{
	}

	[System.MonoTODO("only supported on Windows 2000 SP3 and later")]
	public static void Protect(byte[] userData, MemoryProtectionScope scope)
	{
		if (userData == null)
		{
			throw new ArgumentNullException("userData");
		}
		Check(userData.Length, scope);
		try
		{
			uint cbData = (uint)userData.Length;
			switch (impl)
			{
			case MemoryProtectionImplementation.Win32RtlEncryptMemory:
			{
				int num = RtlEncryptMemory(userData, cbData, (uint)scope);
				if (num < 0)
				{
					throw new CryptographicException(global::Locale.GetText("Error. NTSTATUS = {0}.", num));
				}
				break;
			}
			case MemoryProtectionImplementation.Win32CryptoProtect:
				if (!CryptProtectMemory(userData, cbData, (uint)scope))
				{
					throw new CryptographicException(Marshal.GetLastWin32Error());
				}
				break;
			default:
				throw new PlatformNotSupportedException();
			}
		}
		catch
		{
			impl = MemoryProtectionImplementation.Unsupported;
			throw new PlatformNotSupportedException();
		}
	}

	[System.MonoTODO("only supported on Windows 2000 SP3 and later")]
	public static void Unprotect(byte[] encryptedData, MemoryProtectionScope scope)
	{
		if (encryptedData == null)
		{
			throw new ArgumentNullException("encryptedData");
		}
		Check(encryptedData.Length, scope);
		try
		{
			uint cbData = (uint)encryptedData.Length;
			switch (impl)
			{
			case MemoryProtectionImplementation.Win32RtlEncryptMemory:
			{
				int num = RtlDecryptMemory(encryptedData, cbData, (uint)scope);
				if (num < 0)
				{
					throw new CryptographicException(global::Locale.GetText("Error. NTSTATUS = {0}.", num));
				}
				break;
			}
			case MemoryProtectionImplementation.Win32CryptoProtect:
				if (!CryptUnprotectMemory(encryptedData, cbData, (uint)scope))
				{
					throw new CryptographicException(Marshal.GetLastWin32Error());
				}
				break;
			default:
				throw new PlatformNotSupportedException();
			}
		}
		catch
		{
			impl = MemoryProtectionImplementation.Unsupported;
			throw new PlatformNotSupportedException();
		}
	}

	private static void Detect()
	{
		OperatingSystem oSVersion = Environment.OSVersion;
		PlatformID platform = oSVersion.Platform;
		if (platform == PlatformID.Win32NT)
		{
			Version version = oSVersion.Version;
			if (version.Major < 5)
			{
				impl = MemoryProtectionImplementation.Unsupported;
			}
			else if (version.Major == 5)
			{
				if (version.Minor < 2)
				{
					impl = MemoryProtectionImplementation.Win32RtlEncryptMemory;
				}
				else
				{
					impl = MemoryProtectionImplementation.Win32CryptoProtect;
				}
			}
			else
			{
				impl = MemoryProtectionImplementation.Win32CryptoProtect;
			}
		}
		else
		{
			impl = MemoryProtectionImplementation.Unsupported;
		}
	}

	private static void Check(int size, MemoryProtectionScope scope)
	{
		if (size % 16 != 0)
		{
			throw new CryptographicException(global::Locale.GetText("Not a multiple of {0} bytes.", 16));
		}
		if (scope < MemoryProtectionScope.SameProcess || scope > MemoryProtectionScope.SameLogon)
		{
			throw new ArgumentException(global::Locale.GetText("Invalid enum value for '{0}'.", "MemoryProtectionScope"), "scope");
		}
		switch (impl)
		{
		case MemoryProtectionImplementation.Unknown:
			Detect();
			break;
		case MemoryProtectionImplementation.Unsupported:
			throw new PlatformNotSupportedException();
		}
	}

	[DllImport("advapi32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Auto, EntryPoint = "SystemFunction040", SetLastError = true)]
	[SuppressUnmanagedCodeSecurity]
	private static extern int RtlEncryptMemory(byte[] pData, uint cbData, uint dwFlags);

	[DllImport("advapi32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Auto, EntryPoint = "SystemFunction041", SetLastError = true)]
	[SuppressUnmanagedCodeSecurity]
	private static extern int RtlDecryptMemory(byte[] pData, uint cbData, uint dwFlags);

	[DllImport("crypt32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Auto, SetLastError = true)]
	[SuppressUnmanagedCodeSecurity]
	private static extern bool CryptProtectMemory(byte[] pData, uint cbData, uint dwFlags);

	[DllImport("crypt32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Auto, SetLastError = true)]
	[SuppressUnmanagedCodeSecurity]
	private static extern bool CryptUnprotectMemory(byte[] pData, uint cbData, uint dwFlags);
}

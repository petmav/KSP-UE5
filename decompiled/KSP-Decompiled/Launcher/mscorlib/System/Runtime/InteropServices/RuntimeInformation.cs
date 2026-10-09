using Mono;

namespace System.Runtime.InteropServices;

public static class RuntimeInformation
{
	public static string FrameworkDescription => "Mono " + Mono.Runtime.GetDisplayName();

	public static string OSDescription => Environment.OSVersion.VersionString;

	public static Architecture OSArchitecture
	{
		get
		{
			if (!Environment.Is64BitOperatingSystem)
			{
				return Architecture.X86;
			}
			return Architecture.X64;
		}
	}

	public static Architecture ProcessArchitecture
	{
		get
		{
			if (!Environment.Is64BitProcess)
			{
				return Architecture.X86;
			}
			return Architecture.X64;
		}
	}

	public static bool IsOSPlatform(OSPlatform osPlatform)
	{
		return Environment.Platform switch
		{
			PlatformID.Win32NT => osPlatform == OSPlatform.Windows, 
			PlatformID.MacOSX => osPlatform == OSPlatform.OSX, 
			PlatformID.Unix => osPlatform == OSPlatform.Linux, 
			_ => false, 
		};
	}
}

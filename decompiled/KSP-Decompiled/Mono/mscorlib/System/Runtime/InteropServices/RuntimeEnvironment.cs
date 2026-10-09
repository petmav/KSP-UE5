using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Security;

namespace System.Runtime.InteropServices;

[ComVisible(true)]
public class RuntimeEnvironment
{
	public static string SystemConfigurationFile
	{
		[SecuritySafeCritical]
		get
		{
			return Environment.GetMachineConfigPath();
		}
	}

	[Obsolete("Do not create instances of the RuntimeEnvironment class.  Call the static methods directly on this type instead", true)]
	public RuntimeEnvironment()
	{
	}

	public static bool FromGlobalAccessCache(Assembly a)
	{
		return a.GlobalAssemblyCache;
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	[SecuritySafeCritical]
	public static string GetSystemVersion()
	{
		return Assembly.GetExecutingAssembly().ImageRuntimeVersion;
	}

	[SecuritySafeCritical]
	public static string GetRuntimeDirectory()
	{
		if (Environment.GetEnvironmentVariable("CSC_SDK_PATH_DISABLED") != null)
		{
			return null;
		}
		return GetRuntimeDirectoryImpl();
	}

	private static string GetRuntimeDirectoryImpl()
	{
		return Path.GetDirectoryName(typeof(object).Assembly.Location);
	}

	private static IntPtr GetRuntimeInterfaceImpl(Guid clsid, Guid riid)
	{
		throw new NotSupportedException();
	}

	[SecurityCritical]
	[ComVisible(false)]
	public static IntPtr GetRuntimeInterfaceAsIntPtr(Guid clsid, Guid riid)
	{
		return GetRuntimeInterfaceImpl(clsid, riid);
	}

	[ComVisible(false)]
	[SecurityCritical]
	public static object GetRuntimeInterfaceAsObject(Guid clsid, Guid riid)
	{
		IntPtr intPtr = IntPtr.Zero;
		try
		{
			intPtr = GetRuntimeInterfaceImpl(clsid, riid);
			return Marshal.GetObjectForIUnknown(intPtr);
		}
		finally
		{
			if (intPtr != IntPtr.Zero)
			{
				Marshal.Release(intPtr);
			}
		}
	}
}

using System;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
using System.Security;

namespace Microsoft.Win32;

[SuppressUnmanagedCodeSecurity]
internal static class UnsafeNativeMethods
{
	internal const string KERNEL32 = "kernel32.dll";

	internal const int ERROR_INSUFFICIENT_BUFFER = 122;

	internal const int ERROR_NO_PACKAGE_IDENTITY = 15700;

	[SecuritySafeCritical]
	internal static Lazy<bool> IsPackagedProcess = new Lazy<bool>(() => _IsPackagedProcess());

	[DllImport("kernel32.dll", EntryPoint = "GetCurrentPackageId")]
	[SecurityCritical]
	[return: MarshalAs(UnmanagedType.I4)]
	private static extern int _GetCurrentPackageId(ref int pBufferLength, byte[] pBuffer);

	[DllImport("kernel32.dll", BestFitMapping = false, CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
	private static extern IntPtr GetProcAddress(IntPtr hModule, string methodName);

	[DllImport("kernel32.dll", BestFitMapping = false, CharSet = CharSet.Auto, SetLastError = true)]
	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	private static extern IntPtr GetModuleHandle(string moduleName);

	[SecurityCritical]
	private static bool DoesWin32MethodExist(string moduleName, string methodName)
	{
		IntPtr moduleHandle = GetModuleHandle(moduleName);
		if (moduleHandle == IntPtr.Zero)
		{
			return false;
		}
		return GetProcAddress(moduleHandle, methodName) != IntPtr.Zero;
	}

	[SecuritySafeCritical]
	private static bool _IsPackagedProcess()
	{
		OperatingSystem oSVersion = Environment.OSVersion;
		if (oSVersion.Platform == PlatformID.Win32NT && oSVersion.Version >= new Version(6, 2, 0, 0) && DoesWin32MethodExist("kernel32.dll", "GetCurrentPackageId"))
		{
			int pBufferLength = 0;
			return _GetCurrentPackageId(ref pBufferLength, null) == 122;
		}
		return false;
	}
}

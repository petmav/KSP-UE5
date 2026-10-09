using System.Runtime.InteropServices;

namespace System.Net.NetworkInformation;

internal class Win32NetworkInterface
{
	private static Win32_FIXED_INFO fixedInfo;

	private static bool initialized;

	public static Win32_FIXED_INFO FixedInfo
	{
		get
		{
			if (!initialized)
			{
				int size = 0;
				GetNetworkParams(IntPtr.Zero, ref size);
				IntPtr ptr = Marshal.AllocHGlobal(size);
				GetNetworkParams(ptr, ref size);
				fixedInfo = Marshal.PtrToStructure<Win32_FIXED_INFO>(ptr);
				initialized = true;
			}
			return fixedInfo;
		}
	}

	[DllImport("iphlpapi.dll", SetLastError = true)]
	private static extern int GetNetworkParams(IntPtr ptr, ref int size);
}

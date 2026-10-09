using System.Runtime.InteropServices;

namespace System;

internal static class Platform
{
	private static bool checkedOS;

	private static bool isMacOS;

	private static bool isFreeBSD;

	public static bool IsMacOS
	{
		get
		{
			if (!checkedOS)
			{
				try
				{
					CheckOS();
				}
				catch (DllNotFoundException)
				{
					isMacOS = false;
				}
			}
			return isMacOS;
		}
	}

	public static bool IsFreeBSD
	{
		get
		{
			if (!checkedOS)
			{
				CheckOS();
			}
			return isFreeBSD;
		}
	}

	[DllImport("libc")]
	private static extern int uname(IntPtr buf);

	private static void CheckOS()
	{
		if (Environment.OSVersion.Platform != PlatformID.Unix)
		{
			checkedOS = true;
			return;
		}
		IntPtr intPtr = Marshal.AllocHGlobal(8192);
		try
		{
			if (uname(intPtr) != 0)
			{
				return;
			}
			string text = Marshal.PtrToStringAnsi(intPtr);
			if (!(text == "Darwin"))
			{
				if (text == "FreeBSD")
				{
					isFreeBSD = true;
				}
			}
			else
			{
				isMacOS = true;
			}
		}
		finally
		{
			Marshal.FreeHGlobal(intPtr);
			checkedOS = true;
		}
	}
}

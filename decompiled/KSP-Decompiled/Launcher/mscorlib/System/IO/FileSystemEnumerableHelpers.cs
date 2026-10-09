using System.Security;
using Microsoft.Win32;

namespace System.IO;

internal static class FileSystemEnumerableHelpers
{
	[SecurityCritical]
	internal static bool IsDir(Win32Native.WIN32_FIND_DATA data)
	{
		if (((uint)data.dwFileAttributes & 0x10u) != 0 && !data.cFileName.Equals("."))
		{
			return !data.cFileName.Equals("..");
		}
		return false;
	}

	[SecurityCritical]
	internal static bool IsFile(Win32Native.WIN32_FIND_DATA data)
	{
		return (data.dwFileAttributes & 0x10) == 0;
	}
}

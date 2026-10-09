using System;
using System.Runtime.InteropServices;
using Mono.Net;

namespace Mono.AppleTls;

internal class SecItem
{
	public static readonly IntPtr ReturnRef;

	public static readonly IntPtr MatchSearchList;

	static SecItem()
	{
		IntPtr intPtr = CFObject.dlopen("/System/Library/Frameworks/Security.framework/Security", 0);
		if (intPtr == IntPtr.Zero)
		{
			return;
		}
		try
		{
			ReturnRef = CFObject.GetIntPtr(intPtr, "kSecReturnRef");
			MatchSearchList = CFObject.GetIntPtr(intPtr, "kSecMatchSearchList");
		}
		finally
		{
			CFObject.dlclose(intPtr);
		}
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	internal static extern SecStatusCode SecItemCopyMatching(IntPtr query, out IntPtr result);
}

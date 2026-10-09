using System;
using Mono.Net;

namespace Mono.AppleTls;

internal static class SecClass
{
	public static readonly IntPtr Identity;

	public static readonly IntPtr Certificate;

	static SecClass()
	{
		IntPtr intPtr = CFObject.dlopen("/System/Library/Frameworks/Security.framework/Security", 0);
		if (intPtr == IntPtr.Zero)
		{
			return;
		}
		try
		{
			Identity = CFObject.GetIntPtr(intPtr, "kSecClassIdentity");
			Certificate = CFObject.GetIntPtr(intPtr, "kSecClassCertificate");
		}
		finally
		{
			CFObject.dlclose(intPtr);
		}
	}

	public static IntPtr FromSecKind(SecKind secKind)
	{
		return secKind switch
		{
			SecKind.Identity => Identity, 
			SecKind.Certificate => Certificate, 
			_ => throw new ArgumentException("secKind"), 
		};
	}
}

using System;
using System.Runtime.InteropServices;

namespace XamMac.CoreFoundation;

internal static class CFHelpers
{
	private struct CFRange
	{
		public IntPtr loc;

		public IntPtr len;

		public CFRange(int loc, int len)
			: this((long)loc, (long)len)
		{
		}

		public CFRange(long l, long len)
		{
			loc = (IntPtr)l;
			this.len = (IntPtr)len;
		}
	}

	internal const string CoreFoundationLibrary = "/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation";

	internal const string SecurityLibrary = "/System/Library/Frameworks/Security.framework/Security";

	[DllImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")]
	internal static extern void CFRelease(IntPtr obj);

	[DllImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")]
	internal static extern IntPtr CFRetain(IntPtr obj);

	[DllImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation", CharSet = CharSet.Unicode)]
	private static extern IntPtr CFStringCreateWithCharacters(IntPtr allocator, string str, IntPtr count);

	[DllImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation", CharSet = CharSet.Unicode)]
	private static extern IntPtr CFStringGetLength(IntPtr handle);

	[DllImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation", CharSet = CharSet.Unicode)]
	private static extern IntPtr CFStringGetCharactersPtr(IntPtr handle);

	[DllImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation", CharSet = CharSet.Unicode)]
	private static extern IntPtr CFStringGetCharacters(IntPtr handle, CFRange range, IntPtr buffer);

	internal unsafe static string FetchString(IntPtr handle)
	{
		if (handle == IntPtr.Zero)
		{
			return null;
		}
		int num = (int)CFStringGetLength(handle);
		IntPtr intPtr = CFStringGetCharactersPtr(handle);
		IntPtr intPtr2 = IntPtr.Zero;
		if (intPtr == IntPtr.Zero)
		{
			CFRange range = new CFRange(0, num);
			intPtr2 = Marshal.AllocCoTaskMem(num * 2);
			CFStringGetCharacters(handle, range, intPtr2);
			intPtr = intPtr2;
		}
		string result = new string((char*)(void*)intPtr, 0, num);
		if (intPtr2 != IntPtr.Zero)
		{
			Marshal.FreeCoTaskMem(intPtr2);
		}
		return result;
	}

	[DllImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")]
	private static extern IntPtr CFDataGetLength(IntPtr handle);

	[DllImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")]
	private static extern IntPtr CFDataGetBytePtr(IntPtr handle);

	internal static byte[] FetchDataBuffer(IntPtr handle)
	{
		byte[] array = new byte[(int)CFDataGetLength(handle)];
		Marshal.Copy(CFDataGetBytePtr(handle), array, 0, array.Length);
		return array;
	}

	[DllImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")]
	private static extern IntPtr CFDataCreateWithBytesNoCopy(IntPtr allocator, IntPtr bytes, IntPtr length, IntPtr bytesDeallocator);

	[DllImport("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")]
	private static extern IntPtr CFDataCreate(IntPtr allocator, IntPtr bytes, IntPtr length);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SecCertificateCreateWithData(IntPtr allocator, IntPtr cfData);

	internal unsafe static IntPtr CreateCertificateFromData(byte[] data)
	{
		fixed (byte* ptr = data)
		{
			void* ptr2 = ptr;
			IntPtr intPtr = CFDataCreate(IntPtr.Zero, (IntPtr)ptr2, new IntPtr(data.Length));
			if (intPtr == IntPtr.Zero)
			{
				return IntPtr.Zero;
			}
			IntPtr result = SecCertificateCreateWithData(IntPtr.Zero, intPtr);
			if (intPtr != IntPtr.Zero)
			{
				CFRelease(intPtr);
			}
			return result;
		}
	}
}

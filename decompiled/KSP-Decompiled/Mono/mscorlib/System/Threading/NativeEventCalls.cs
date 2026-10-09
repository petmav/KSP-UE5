using System.Runtime.CompilerServices;
using System.Security.AccessControl;
using Microsoft.Win32.SafeHandles;

namespace System.Threading;

internal static class NativeEventCalls
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern IntPtr CreateEvent_internal(bool manual, bool initial, string name, out int errorCode);

	public static bool SetEvent(SafeWaitHandle handle)
	{
		bool success = false;
		try
		{
			handle.DangerousAddRef(ref success);
			return SetEvent_internal(handle.DangerousGetHandle());
		}
		finally
		{
			if (success)
			{
				handle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool SetEvent_internal(IntPtr handle);

	public static bool ResetEvent(SafeWaitHandle handle)
	{
		bool success = false;
		try
		{
			handle.DangerousAddRef(ref success);
			return ResetEvent_internal(handle.DangerousGetHandle());
		}
		finally
		{
			if (success)
			{
				handle.DangerousRelease();
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool ResetEvent_internal(IntPtr handle);

	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern void CloseEvent_internal(IntPtr handle);

	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern IntPtr OpenEvent_internal(string name, EventWaitHandleRights rights, out int errorCode);
}

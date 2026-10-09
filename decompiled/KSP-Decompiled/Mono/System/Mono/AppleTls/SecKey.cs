using System;
using System.Runtime.InteropServices;
using Mono.Net;
using ObjCRuntimeInternal;

namespace Mono.AppleTls;

internal class SecKey : INativeObject, IDisposable
{
	internal IntPtr handle;

	internal IntPtr owner;

	public IntPtr Handle => handle;

	public SecKey(IntPtr handle, bool owns = false)
	{
		this.handle = handle;
		if (!owns)
		{
			CFObject.CFRetain(handle);
		}
	}

	internal SecKey(IntPtr handle, IntPtr owner)
	{
		this.handle = handle;
		this.owner = owner;
		CFObject.CFRetain(owner);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security", EntryPoint = "SecKeyGetTypeID")]
	public static extern IntPtr GetTypeID();

	~SecKey()
	{
		Dispose(disposing: false);
	}

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (owner != IntPtr.Zero)
		{
			CFObject.CFRelease(owner);
			owner = (handle = IntPtr.Zero);
		}
		else if (handle != IntPtr.Zero)
		{
			CFObject.CFRelease(handle);
			handle = IntPtr.Zero;
		}
	}
}

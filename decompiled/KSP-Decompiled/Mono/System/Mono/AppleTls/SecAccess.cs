using System;
using System.Runtime.InteropServices;
using Mono.Net;
using ObjCRuntimeInternal;

namespace Mono.AppleTls;

internal class SecAccess : INativeObject, IDisposable
{
	internal IntPtr handle;

	public IntPtr Handle => handle;

	public SecAccess(IntPtr handle, bool owns = false)
	{
		this.handle = handle;
		if (!owns)
		{
			CFObject.CFRetain(handle);
		}
	}

	~SecAccess()
	{
		Dispose(disposing: false);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SecStatusCode SecAccessCreate(IntPtr descriptor, IntPtr trustedList, out IntPtr accessRef);

	public static SecAccess Create(string descriptor)
	{
		CFString cFString = CFString.Create(descriptor);
		if (cFString == null)
		{
			throw new InvalidOperationException();
		}
		try
		{
			IntPtr accessRef;
			SecStatusCode secStatusCode = SecAccessCreate(cFString.Handle, IntPtr.Zero, out accessRef);
			if (secStatusCode != 0)
			{
				throw new InvalidOperationException(secStatusCode.ToString());
			}
			return new SecAccess(accessRef, owns: true);
		}
		finally
		{
			cFString.Dispose();
		}
	}

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (handle != IntPtr.Zero)
		{
			CFObject.CFRelease(handle);
			handle = IntPtr.Zero;
		}
	}
}

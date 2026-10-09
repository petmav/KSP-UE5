using System;
using System.Runtime.InteropServices;
using Mono.Net;
using ObjCRuntimeInternal;

namespace Mono.AppleTls;

internal class SecPolicy : INativeObject, IDisposable
{
	private IntPtr handle;

	public IntPtr Handle => handle;

	internal SecPolicy(IntPtr handle, bool owns = false)
	{
		if (handle == IntPtr.Zero)
		{
			throw new Exception("Invalid handle");
		}
		this.handle = handle;
		if (!owns)
		{
			CFObject.CFRetain(handle);
		}
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SecPolicyCreateSSL(bool server, IntPtr hostname);

	public static SecPolicy CreateSslPolicy(bool server, string hostName)
	{
		CFString cFString = ((hostName == null) ? null : CFString.Create(hostName));
		IntPtr hostname = cFString?.Handle ?? IntPtr.Zero;
		SecPolicy result = new SecPolicy(SecPolicyCreateSSL(server, hostname), owns: true);
		cFString?.Dispose();
		return result;
	}

	~SecPolicy()
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
		if (handle != IntPtr.Zero)
		{
			CFObject.CFRelease(handle);
			handle = IntPtr.Zero;
		}
	}
}

using System;
using System.IO;
using System.Security.Permissions;

namespace Microsoft.Win32.SafeHandles;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
[SecurityPermission(SecurityAction.LinkDemand, UnmanagedCode = true)]
public sealed class SafePipeHandle : SafeHandleZeroOrMinusOneIsInvalid
{
	public SafePipeHandle(IntPtr preexistingHandle, bool ownsHandle)
		: base(ownsHandle)
	{
		handle = preexistingHandle;
	}

	protected override bool ReleaseHandle()
	{
		MonoIOError error;
		return MonoIO.Close(handle, out error);
	}
}

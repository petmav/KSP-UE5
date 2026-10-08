using System;
using System.Security.Permissions;
using Unity;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class TimerElapsedEventArgs : EventArgs
{
	public IntPtr TimerId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(IntPtr);
		}
	}

	public TimerElapsedEventArgs(IntPtr timerId)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

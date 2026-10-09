using System;
using System.Security.Permissions;
using Unity;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class SessionSwitchEventArgs : EventArgs
{
	public SessionSwitchReason Reason
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SessionSwitchReason);
		}
	}

	public SessionSwitchEventArgs(SessionSwitchReason reason)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

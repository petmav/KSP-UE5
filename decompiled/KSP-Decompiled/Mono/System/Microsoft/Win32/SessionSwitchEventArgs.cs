using System;
using System.Security.Permissions;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
[PermissionSet(SecurityAction.InheritanceDemand, Unrestricted = true)]
public class SessionSwitchEventArgs : EventArgs
{
	private SessionSwitchReason reason;

	public SessionSwitchReason Reason => reason;

	public SessionSwitchEventArgs(SessionSwitchReason reason)
	{
		this.reason = reason;
	}
}

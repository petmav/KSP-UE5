using System;
using System.Security.Permissions;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
[PermissionSet(SecurityAction.InheritanceDemand, Unrestricted = true)]
public class SessionEndedEventArgs : EventArgs
{
	private SessionEndReasons myreason;

	public SessionEndReasons Reason => myreason;

	public SessionEndedEventArgs(SessionEndReasons reason)
	{
		myreason = reason;
	}
}

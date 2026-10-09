using System;
using System.Security.Permissions;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
[PermissionSet(SecurityAction.InheritanceDemand, Unrestricted = true)]
public class SessionEndingEventArgs : EventArgs
{
	private SessionEndReasons myreason;

	private bool mycancel;

	public SessionEndReasons Reason => myreason;

	public bool Cancel
	{
		get
		{
			return mycancel;
		}
		set
		{
			mycancel = value;
		}
	}

	public SessionEndingEventArgs(SessionEndReasons reason)
	{
		myreason = reason;
	}
}

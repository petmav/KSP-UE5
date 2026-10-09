using System;
using System.Security.Permissions;
using Unity;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class SessionEndingEventArgs : EventArgs
{
	public bool Cancel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public SessionEndReasons Reason
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SessionEndReasons);
		}
	}

	public SessionEndingEventArgs(SessionEndReasons reason)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System;
using System.Security.Permissions;
using Unity;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class SessionEndedEventArgs : EventArgs
{
	public SessionEndReasons Reason
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SessionEndReasons);
		}
	}

	public SessionEndedEventArgs(SessionEndReasons reason)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System;
using System.Security.Permissions;
using Unity;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class UserPreferenceChangingEventArgs : EventArgs
{
	public UserPreferenceCategory Category
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(UserPreferenceCategory);
		}
	}

	public UserPreferenceChangingEventArgs(UserPreferenceCategory category)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

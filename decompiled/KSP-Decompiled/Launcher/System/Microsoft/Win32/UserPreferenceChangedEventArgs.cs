using System;
using System.Security.Permissions;
using Unity;

namespace Microsoft.Win32;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public class UserPreferenceChangedEventArgs : EventArgs
{
	public UserPreferenceCategory Category
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(UserPreferenceCategory);
		}
	}

	public UserPreferenceChangedEventArgs(UserPreferenceCategory category)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

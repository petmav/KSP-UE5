using System;
using System.Security.Permissions;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
[PermissionSet(SecurityAction.InheritanceDemand, Unrestricted = true)]
public class UserPreferenceChangingEventArgs : EventArgs
{
	private UserPreferenceCategory mycategory;

	public UserPreferenceCategory Category => mycategory;

	public UserPreferenceChangingEventArgs(UserPreferenceCategory category)
	{
		mycategory = category;
	}
}

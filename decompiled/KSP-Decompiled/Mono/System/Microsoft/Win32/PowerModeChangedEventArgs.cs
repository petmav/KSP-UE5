using System;
using System.Security.Permissions;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
[PermissionSet(SecurityAction.InheritanceDemand, Unrestricted = true)]
public class PowerModeChangedEventArgs : EventArgs
{
	private PowerModes mymode;

	public PowerModes Mode => mymode;

	public PowerModeChangedEventArgs(PowerModes mode)
	{
		mymode = mode;
	}
}

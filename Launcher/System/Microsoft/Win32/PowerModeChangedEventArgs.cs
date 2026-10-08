using System;
using System.Security.Permissions;
using Unity;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class PowerModeChangedEventArgs : EventArgs
{
	public PowerModes Mode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PowerModes);
		}
	}

	public PowerModeChangedEventArgs(PowerModes mode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System;
using System.Security.Permissions;

namespace Microsoft.Win32;

[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
[PermissionSet(SecurityAction.InheritanceDemand, Unrestricted = true)]
public class TimerElapsedEventArgs : EventArgs
{
	private IntPtr mytimerId;

	public IntPtr TimerId => mytimerId;

	public TimerElapsedEventArgs(IntPtr timerId)
	{
		mytimerId = timerId;
	}
}

using System;
using System.Security.Permissions;
using Unity;

namespace Microsoft.Win32;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public sealed class SystemEvents
{
	public static event EventHandler DisplaySettingsChanged
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event EventHandler DisplaySettingsChanging
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event EventHandler EventsThreadShutdown
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event EventHandler InstalledFontsChanged
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event EventHandler LowMemory
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event EventHandler PaletteChanged
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event PowerModeChangedEventHandler PowerModeChanged
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event SessionEndedEventHandler SessionEnded
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event SessionEndingEventHandler SessionEnding
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event SessionSwitchEventHandler SessionSwitch
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event EventHandler TimeChanged
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event TimerElapsedEventHandler TimerElapsed
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event UserPreferenceChangedEventHandler UserPreferenceChanged
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static event UserPreferenceChangingEventHandler UserPreferenceChanging
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	internal SystemEvents()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static IntPtr CreateTimer(int interval)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	public static void InvokeOnEventsThread(Delegate method)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void KillTimer(IntPtr timerId)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

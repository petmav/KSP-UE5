using System.Security.Permissions;

namespace System.ComponentModel;

[HostProtection(SecurityAction.LinkDemand, SharedState = true)]
public class CancelEventArgs : EventArgs
{
	private bool cancel;

	public bool Cancel
	{
		get
		{
			return cancel;
		}
		set
		{
			cancel = value;
		}
	}

	public CancelEventArgs()
		: this(cancel: false)
	{
	}

	public CancelEventArgs(bool cancel)
	{
		this.cancel = cancel;
	}
}

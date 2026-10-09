using System.Security.Permissions;

namespace System.ComponentModel;

[HostProtection(SecurityAction.LinkDemand, SharedState = true)]
public class RunWorkerCompletedEventArgs : AsyncCompletedEventArgs
{
	private object result;

	public object Result
	{
		get
		{
			RaiseExceptionIfNecessary();
			return result;
		}
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	[Browsable(false)]
	public new object UserState => base.UserState;

	public RunWorkerCompletedEventArgs(object result, Exception error, bool cancelled)
		: base(error, cancelled, null)
	{
		this.result = result;
	}
}

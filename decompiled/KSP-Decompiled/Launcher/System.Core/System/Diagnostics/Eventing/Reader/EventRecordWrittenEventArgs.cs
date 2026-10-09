using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing.Reader;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class EventRecordWrittenEventArgs : EventArgs
{
	public Exception EventException
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public EventRecord EventRecord
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal EventRecordWrittenEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

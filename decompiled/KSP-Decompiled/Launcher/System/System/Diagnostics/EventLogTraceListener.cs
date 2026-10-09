using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[HostProtection(SecurityAction.LinkDemand, Synchronization = true)]
public sealed class EventLogTraceListener : TraceListener
{
	public EventLog EventLog
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public EventLogTraceListener()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogTraceListener(EventLog eventLog)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogTraceListener(string source)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Write(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void WriteLine(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

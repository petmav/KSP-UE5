using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventProviderTraceListener : TraceListener
{
	public string Delimiter
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

	public EventProviderTraceListener(string providerId)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventProviderTraceListener(string providerId, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventProviderTraceListener(string providerId, string name, string delimiter)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public sealed override void Write(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public sealed override void WriteLine(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

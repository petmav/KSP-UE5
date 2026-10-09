using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing.Reader;

[Serializable]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventLogNotFoundException : EventLogException
{
	public EventLogNotFoundException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected EventLogNotFoundException(SerializationInfo serializationInfo, StreamingContext streamingContext)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogNotFoundException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogNotFoundException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

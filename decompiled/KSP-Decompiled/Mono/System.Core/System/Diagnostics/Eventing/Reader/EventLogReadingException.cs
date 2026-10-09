using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing.Reader;

[Serializable]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventLogReadingException : EventLogException
{
	public EventLogReadingException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected EventLogReadingException(SerializationInfo serializationInfo, StreamingContext streamingContext)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogReadingException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogReadingException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

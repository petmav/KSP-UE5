using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing.Reader;

[Serializable]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventLogException : Exception
{
	public EventLogException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected EventLogException(int errorCode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected EventLogException(SerializationInfo serializationInfo, StreamingContext streamingContext)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

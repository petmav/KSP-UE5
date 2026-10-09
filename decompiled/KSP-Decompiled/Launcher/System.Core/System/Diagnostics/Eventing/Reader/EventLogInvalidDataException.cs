using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing.Reader;

[Serializable]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventLogInvalidDataException : EventLogException
{
	public EventLogInvalidDataException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected EventLogInvalidDataException(SerializationInfo serializationInfo, StreamingContext streamingContext)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogInvalidDataException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogInvalidDataException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

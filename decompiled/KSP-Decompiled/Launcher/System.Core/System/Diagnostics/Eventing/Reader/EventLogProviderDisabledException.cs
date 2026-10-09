using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing.Reader;

[Serializable]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventLogProviderDisabledException : EventLogException
{
	public EventLogProviderDisabledException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected EventLogProviderDisabledException(SerializationInfo serializationInfo, StreamingContext streamingContext)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogProviderDisabledException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogProviderDisabledException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Management.Instrumentation;

[Serializable]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class InstanceNotFoundException : InstrumentationException
{
	public InstanceNotFoundException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected InstanceNotFoundException(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public InstanceNotFoundException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public InstanceNotFoundException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

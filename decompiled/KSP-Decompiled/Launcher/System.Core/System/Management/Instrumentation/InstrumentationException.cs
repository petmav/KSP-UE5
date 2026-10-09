using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Management.Instrumentation;

[Serializable]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class InstrumentationException : InstrumentationBaseException
{
	public InstrumentationException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public InstrumentationException(Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected InstrumentationException(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public InstrumentationException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public InstrumentationException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

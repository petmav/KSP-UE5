using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Management.Instrumentation;

[Serializable]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class InstrumentationBaseException : Exception
{
	public InstrumentationBaseException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected InstrumentationBaseException(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public InstrumentationBaseException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public InstrumentationBaseException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

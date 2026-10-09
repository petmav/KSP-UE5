using System.Runtime.Serialization;
using System.Security.Permissions;

namespace System.Threading;

[Serializable]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class LockRecursionException : Exception
{
	public LockRecursionException()
	{
	}

	public LockRecursionException(string message)
		: base(message)
	{
	}

	protected LockRecursionException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}

	public LockRecursionException(string message, Exception innerException)
		: base(message, innerException)
	{
	}
}

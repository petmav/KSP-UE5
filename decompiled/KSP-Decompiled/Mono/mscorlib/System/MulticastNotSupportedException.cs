using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public sealed class MulticastNotSupportedException : SystemException
{
	public MulticastNotSupportedException()
		: base(Environment.GetResourceString("Attempted to add multiple callbacks to a delegate that does not support multicast."))
	{
		SetErrorCode(-2146233068);
	}

	public MulticastNotSupportedException(string message)
		: base(message)
	{
		SetErrorCode(-2146233068);
	}

	public MulticastNotSupportedException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233068);
	}

	internal MulticastNotSupportedException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

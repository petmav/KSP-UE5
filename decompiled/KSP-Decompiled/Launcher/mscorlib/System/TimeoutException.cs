using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class TimeoutException : SystemException
{
	public TimeoutException()
		: base(Environment.GetResourceString("The operation has timed out."))
	{
		SetErrorCode(-2146233083);
	}

	public TimeoutException(string message)
		: base(message)
	{
		SetErrorCode(-2146233083);
	}

	public TimeoutException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146233083);
	}

	protected TimeoutException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

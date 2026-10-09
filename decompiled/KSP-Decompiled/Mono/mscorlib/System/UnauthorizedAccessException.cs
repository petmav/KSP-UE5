using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class UnauthorizedAccessException : SystemException
{
	public UnauthorizedAccessException()
		: base(Environment.GetResourceString("Attempted to perform an unauthorized operation."))
	{
		SetErrorCode(-2147024891);
	}

	public UnauthorizedAccessException(string message)
		: base(message)
	{
		SetErrorCode(-2147024891);
	}

	public UnauthorizedAccessException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2147024891);
	}

	protected UnauthorizedAccessException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

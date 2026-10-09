using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class InvalidCastException : SystemException
{
	public InvalidCastException()
		: base(Environment.GetResourceString("Specified cast is not valid."))
	{
		SetErrorCode(-2147467262);
	}

	public InvalidCastException(string message)
		: base(message)
	{
		SetErrorCode(-2147467262);
	}

	public InvalidCastException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2147467262);
	}

	protected InvalidCastException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}

	public InvalidCastException(string message, int errorCode)
		: base(message)
	{
		SetErrorCode(errorCode);
	}
}

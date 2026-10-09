using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class OutOfMemoryException : SystemException
{
	public OutOfMemoryException()
		: base(Exception.GetMessageFromNativeResources(ExceptionMessageKind.OutOfMemory))
	{
		SetErrorCode(-2147024882);
	}

	public OutOfMemoryException(string message)
		: base(message)
	{
		SetErrorCode(-2147024882);
	}

	public OutOfMemoryException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2147024882);
	}

	protected OutOfMemoryException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

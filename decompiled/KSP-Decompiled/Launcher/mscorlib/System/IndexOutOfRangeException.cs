using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public sealed class IndexOutOfRangeException : SystemException
{
	public IndexOutOfRangeException()
		: base(Environment.GetResourceString("Index was outside the bounds of the array."))
	{
		SetErrorCode(-2146233080);
	}

	public IndexOutOfRangeException(string message)
		: base(message)
	{
		SetErrorCode(-2146233080);
	}

	public IndexOutOfRangeException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146233080);
	}

	internal IndexOutOfRangeException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

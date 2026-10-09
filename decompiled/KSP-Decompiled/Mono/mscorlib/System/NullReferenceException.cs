using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class NullReferenceException : SystemException
{
	public NullReferenceException()
		: base(Environment.GetResourceString("Object reference not set to an instance of an object."))
	{
		SetErrorCode(-2147467261);
	}

	public NullReferenceException(string message)
		: base(message)
	{
		SetErrorCode(-2147467261);
	}

	public NullReferenceException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2147467261);
	}

	protected NullReferenceException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

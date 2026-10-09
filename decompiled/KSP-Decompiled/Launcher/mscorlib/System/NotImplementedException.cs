using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class NotImplementedException : SystemException
{
	public NotImplementedException()
		: base(Environment.GetResourceString("The method or operation is not implemented."))
	{
		SetErrorCode(-2147467263);
	}

	public NotImplementedException(string message)
		: base(message)
	{
		SetErrorCode(-2147467263);
	}

	public NotImplementedException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2147467263);
	}

	protected NotImplementedException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

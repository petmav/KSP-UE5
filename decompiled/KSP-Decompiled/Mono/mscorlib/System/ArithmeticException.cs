using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class ArithmeticException : SystemException
{
	public ArithmeticException()
		: base(Environment.GetResourceString("Overflow or underflow in the arithmetic operation."))
	{
		SetErrorCode(-2147024362);
	}

	public ArithmeticException(string message)
		: base(message)
	{
		SetErrorCode(-2147024362);
	}

	public ArithmeticException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2147024362);
	}

	protected ArithmeticException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

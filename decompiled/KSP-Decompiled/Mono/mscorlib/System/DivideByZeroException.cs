using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class DivideByZeroException : ArithmeticException
{
	public DivideByZeroException()
		: base(Environment.GetResourceString("Attempted to divide by zero."))
	{
		SetErrorCode(-2147352558);
	}

	public DivideByZeroException(string message)
		: base(message)
	{
		SetErrorCode(-2147352558);
	}

	public DivideByZeroException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2147352558);
	}

	protected DivideByZeroException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

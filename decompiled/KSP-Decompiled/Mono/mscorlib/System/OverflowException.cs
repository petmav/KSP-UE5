using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class OverflowException : ArithmeticException
{
	public OverflowException()
		: base(Environment.GetResourceString("Arithmetic operation resulted in an overflow."))
	{
		SetErrorCode(-2146233066);
	}

	public OverflowException(string message)
		: base(message)
	{
		SetErrorCode(-2146233066);
	}

	public OverflowException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146233066);
	}

	protected OverflowException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

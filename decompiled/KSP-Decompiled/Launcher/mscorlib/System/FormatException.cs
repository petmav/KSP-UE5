using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class FormatException : SystemException
{
	public FormatException()
		: base(Environment.GetResourceString("One of the identified items was in an invalid format."))
	{
		SetErrorCode(-2146233033);
	}

	public FormatException(string message)
		: base(message)
	{
		SetErrorCode(-2146233033);
	}

	public FormatException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146233033);
	}

	protected FormatException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

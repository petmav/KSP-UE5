using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public sealed class InvalidProgramException : SystemException
{
	public InvalidProgramException()
		: base(Environment.GetResourceString("Common Language Runtime detected an invalid program."))
	{
		SetErrorCode(-2146233030);
	}

	public InvalidProgramException(string message)
		: base(message)
	{
		SetErrorCode(-2146233030);
	}

	public InvalidProgramException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233030);
	}

	internal InvalidProgramException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

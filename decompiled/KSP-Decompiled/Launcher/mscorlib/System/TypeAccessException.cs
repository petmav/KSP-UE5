using System.Runtime.Serialization;

namespace System;

[Serializable]
public class TypeAccessException : TypeLoadException
{
	public TypeAccessException()
		: base(Environment.GetResourceString("Attempt to access the type failed."))
	{
		SetErrorCode(-2146233021);
	}

	public TypeAccessException(string message)
		: base(message)
	{
		SetErrorCode(-2146233021);
	}

	public TypeAccessException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233021);
	}

	protected TypeAccessException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
		SetErrorCode(-2146233021);
	}
}

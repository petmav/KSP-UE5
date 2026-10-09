using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class MemberAccessException : SystemException
{
	public MemberAccessException()
		: base(Environment.GetResourceString("Cannot access member."))
	{
		SetErrorCode(-2146233062);
	}

	public MemberAccessException(string message)
		: base(message)
	{
		SetErrorCode(-2146233062);
	}

	public MemberAccessException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233062);
	}

	protected MemberAccessException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

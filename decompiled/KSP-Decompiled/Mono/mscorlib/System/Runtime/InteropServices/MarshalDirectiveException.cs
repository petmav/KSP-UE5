using System.Runtime.Serialization;

namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
public class MarshalDirectiveException : SystemException
{
	public MarshalDirectiveException()
		: base(Environment.GetResourceString("Marshaling directives are invalid."))
	{
		SetErrorCode(-2146233035);
	}

	public MarshalDirectiveException(string message)
		: base(message)
	{
		SetErrorCode(-2146233035);
	}

	public MarshalDirectiveException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233035);
	}

	protected MarshalDirectiveException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

using System.Runtime.Serialization;

namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
public class SafeArrayTypeMismatchException : SystemException
{
	public SafeArrayTypeMismatchException()
		: base(Environment.GetResourceString("Specified array was not of the expected type."))
	{
		SetErrorCode(-2146233037);
	}

	public SafeArrayTypeMismatchException(string message)
		: base(message)
	{
		SetErrorCode(-2146233037);
	}

	public SafeArrayTypeMismatchException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233037);
	}

	protected SafeArrayTypeMismatchException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

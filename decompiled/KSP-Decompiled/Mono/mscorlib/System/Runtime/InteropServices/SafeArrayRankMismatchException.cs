using System.Runtime.Serialization;

namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
public class SafeArrayRankMismatchException : SystemException
{
	public SafeArrayRankMismatchException()
		: base(Environment.GetResourceString("Specified array was not of the expected rank."))
	{
		SetErrorCode(-2146233032);
	}

	public SafeArrayRankMismatchException(string message)
		: base(message)
	{
		SetErrorCode(-2146233032);
	}

	public SafeArrayRankMismatchException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233032);
	}

	protected SafeArrayRankMismatchException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

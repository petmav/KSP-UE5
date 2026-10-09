using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
public sealed class AmbiguousMatchException : SystemException
{
	public AmbiguousMatchException()
		: base(Environment.GetResourceString("Ambiguous match found."))
	{
		SetErrorCode(-2147475171);
	}

	public AmbiguousMatchException(string message)
		: base(message)
	{
		SetErrorCode(-2147475171);
	}

	public AmbiguousMatchException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2147475171);
	}

	internal AmbiguousMatchException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

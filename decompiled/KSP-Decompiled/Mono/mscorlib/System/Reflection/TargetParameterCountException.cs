using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
public sealed class TargetParameterCountException : ApplicationException
{
	public TargetParameterCountException()
		: base(Environment.GetResourceString("Number of parameters specified does not match the expected number."))
	{
		SetErrorCode(-2147352562);
	}

	public TargetParameterCountException(string message)
		: base(message)
	{
		SetErrorCode(-2147352562);
	}

	public TargetParameterCountException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2147352562);
	}

	internal TargetParameterCountException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

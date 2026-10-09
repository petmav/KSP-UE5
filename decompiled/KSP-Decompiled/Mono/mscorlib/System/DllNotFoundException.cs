using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class DllNotFoundException : TypeLoadException
{
	public DllNotFoundException()
		: base(Environment.GetResourceString("Dll was not found."))
	{
		SetErrorCode(-2146233052);
	}

	public DllNotFoundException(string message)
		: base(message)
	{
		SetErrorCode(-2146233052);
	}

	public DllNotFoundException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233052);
	}

	protected DllNotFoundException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

[Serializable]
[ComVisible(true)]
public class ArgumentNullException : ArgumentException
{
	public ArgumentNullException()
		: base(Environment.GetResourceString("Value cannot be null."))
	{
		SetErrorCode(-2147467261);
	}

	public ArgumentNullException(string paramName)
		: base(Environment.GetResourceString("Value cannot be null."), paramName)
	{
		SetErrorCode(-2147467261);
	}

	public ArgumentNullException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2147467261);
	}

	public ArgumentNullException(string paramName, string message)
		: base(message, paramName)
	{
		SetErrorCode(-2147467261);
	}

	[SecurityCritical]
	protected ArgumentNullException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

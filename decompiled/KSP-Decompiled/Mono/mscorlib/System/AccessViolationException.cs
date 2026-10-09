using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System;

[Serializable]
[ComVisible(true)]
public class AccessViolationException : SystemException
{
	private IntPtr _ip;

	private IntPtr _target;

	private int _accessType;

	public AccessViolationException()
		: base(Environment.GetResourceString("Attempted to read or write protected memory. This is often an indication that other memory is corrupt."))
	{
		SetErrorCode(-2147467261);
	}

	public AccessViolationException(string message)
		: base(message)
	{
		SetErrorCode(-2147467261);
	}

	public AccessViolationException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2147467261);
	}

	protected AccessViolationException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

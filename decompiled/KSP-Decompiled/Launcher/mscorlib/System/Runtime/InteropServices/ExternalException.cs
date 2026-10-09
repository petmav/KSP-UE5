using System.Globalization;
using System.Runtime.Serialization;

namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
public class ExternalException : SystemException
{
	public virtual int ErrorCode => base.HResult;

	public ExternalException()
		: base(Environment.GetResourceString("External component has thrown an exception."))
	{
		SetErrorCode(-2147467259);
	}

	public ExternalException(string message)
		: base(message)
	{
		SetErrorCode(-2147467259);
	}

	public ExternalException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2147467259);
	}

	public ExternalException(string message, int errorCode)
		: base(message)
	{
		SetErrorCode(errorCode);
	}

	protected ExternalException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}

	public override string ToString()
	{
		string message = Message;
		string text = GetType().ToString() + " (0x" + base.HResult.ToString("X8", CultureInfo.InvariantCulture) + ")";
		if (!string.IsNullOrEmpty(message))
		{
			text = text + ": " + message;
		}
		Exception innerException = base.InnerException;
		if (innerException != null)
		{
			text = text + " ---> " + innerException.ToString();
		}
		if (StackTrace != null)
		{
			text = text + Environment.NewLine + StackTrace;
		}
		return text;
	}
}

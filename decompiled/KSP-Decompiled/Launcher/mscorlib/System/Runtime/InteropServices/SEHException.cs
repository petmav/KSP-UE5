using System.Runtime.Serialization;

namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
public class SEHException : ExternalException
{
	public SEHException()
	{
		SetErrorCode(-2147467259);
	}

	public SEHException(string message)
		: base(message)
	{
		SetErrorCode(-2147467259);
	}

	public SEHException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2147467259);
	}

	protected SEHException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}

	public virtual bool CanResume()
	{
		return false;
	}
}

using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System.Threading;

[Serializable]
[ComVisible(false)]
public class WaitHandleCannotBeOpenedException : ApplicationException
{
	public WaitHandleCannotBeOpenedException()
		: base(Environment.GetResourceString("No handle of the given name exists."))
	{
		SetErrorCode(-2146233044);
	}

	public WaitHandleCannotBeOpenedException(string message)
		: base(message)
	{
		SetErrorCode(-2146233044);
	}

	public WaitHandleCannotBeOpenedException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146233044);
	}

	protected WaitHandleCannotBeOpenedException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

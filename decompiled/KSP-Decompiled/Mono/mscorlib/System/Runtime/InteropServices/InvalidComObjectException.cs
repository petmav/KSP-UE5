using System.Runtime.Serialization;

namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
public class InvalidComObjectException : SystemException
{
	public InvalidComObjectException()
		: base(Environment.GetResourceString("Attempt has been made to use a COM object that does not have a backing class factory."))
	{
		SetErrorCode(-2146233049);
	}

	public InvalidComObjectException(string message)
		: base(message)
	{
		SetErrorCode(-2146233049);
	}

	public InvalidComObjectException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233049);
	}

	protected InvalidComObjectException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

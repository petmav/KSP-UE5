using System.Runtime.InteropServices;

namespace System.Runtime.Serialization;

[Serializable]
[ComVisible(true)]
public class SerializationException : SystemException
{
	private static string _nullMessage = Environment.GetResourceString("Serialization error.");

	public SerializationException()
		: base(_nullMessage)
	{
		SetErrorCode(-2146233076);
	}

	public SerializationException(string message)
		: base(message)
	{
		SetErrorCode(-2146233076);
	}

	public SerializationException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146233076);
	}

	protected SerializationException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

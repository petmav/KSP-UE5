using System.Runtime.Serialization;

namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
public class InvalidOleVariantTypeException : SystemException
{
	public InvalidOleVariantTypeException()
		: base(Environment.GetResourceString("Specified OLE variant was invalid."))
	{
		SetErrorCode(-2146233039);
	}

	public InvalidOleVariantTypeException(string message)
		: base(message)
	{
		SetErrorCode(-2146233039);
	}

	public InvalidOleVariantTypeException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233039);
	}

	protected InvalidOleVariantTypeException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

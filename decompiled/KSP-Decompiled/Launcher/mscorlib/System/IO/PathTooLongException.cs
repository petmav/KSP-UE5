using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System.IO;

[Serializable]
[ComVisible(true)]
public class PathTooLongException : IOException
{
	public PathTooLongException()
		: base(Environment.GetResourceString("The specified path, file name, or both are too long. The fully qualified file name must be less than 260 characters, and the directory name must be less than 248 characters."))
	{
		SetErrorCode(-2147024690);
	}

	public PathTooLongException(string message)
		: base(message)
	{
		SetErrorCode(-2147024690);
	}

	public PathTooLongException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2147024690);
	}

	protected PathTooLongException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

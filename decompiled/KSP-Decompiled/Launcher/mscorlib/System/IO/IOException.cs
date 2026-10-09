using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System.IO;

[Serializable]
[ComVisible(true)]
public class IOException : SystemException
{
	[NonSerialized]
	private string _maybeFullPath;

	public IOException()
		: base(Environment.GetResourceString("I/O error occurred."))
	{
		SetErrorCode(-2146232800);
	}

	public IOException(string message)
		: base(message)
	{
		SetErrorCode(-2146232800);
	}

	public IOException(string message, int hresult)
		: base(message)
	{
		SetErrorCode(hresult);
	}

	internal IOException(string message, int hresult, string maybeFullPath)
		: base(message)
	{
		SetErrorCode(hresult);
		_maybeFullPath = maybeFullPath;
	}

	public IOException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146232800);
	}

	protected IOException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

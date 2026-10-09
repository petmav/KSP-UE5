using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System.Resources;

[Serializable]
[ComVisible(true)]
public class MissingManifestResourceException : SystemException
{
	public MissingManifestResourceException()
		: base(Environment.GetResourceString("Unable to find manifest resource."))
	{
		SetErrorCode(-2146233038);
	}

	public MissingManifestResourceException(string message)
		: base(message)
	{
		SetErrorCode(-2146233038);
	}

	public MissingManifestResourceException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233038);
	}

	protected MissingManifestResourceException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

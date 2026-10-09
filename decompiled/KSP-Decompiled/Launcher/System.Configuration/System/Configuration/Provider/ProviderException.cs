using System.Runtime.Serialization;
using Unity;

namespace System.Configuration.Provider;

[Serializable]
public class ProviderException : Exception
{
	public ProviderException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected ProviderException(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ProviderException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ProviderException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

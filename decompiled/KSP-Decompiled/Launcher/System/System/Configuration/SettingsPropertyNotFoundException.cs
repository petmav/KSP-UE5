using System.Runtime.Serialization;
using Unity;

namespace System.Configuration;

[Serializable]
public class SettingsPropertyNotFoundException : Exception
{
	public SettingsPropertyNotFoundException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected SettingsPropertyNotFoundException(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SettingsPropertyNotFoundException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SettingsPropertyNotFoundException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

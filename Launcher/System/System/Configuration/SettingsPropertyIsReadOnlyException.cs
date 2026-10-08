using System.Runtime.Serialization;
using Unity;

namespace System.Configuration;

[Serializable]
public class SettingsPropertyIsReadOnlyException : Exception
{
	public SettingsPropertyIsReadOnlyException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected SettingsPropertyIsReadOnlyException(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SettingsPropertyIsReadOnlyException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SettingsPropertyIsReadOnlyException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

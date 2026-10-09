using System.Runtime.Serialization;
using Unity;

namespace System.Configuration;

[Serializable]
public class SettingsPropertyWrongTypeException : Exception
{
	public SettingsPropertyWrongTypeException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected SettingsPropertyWrongTypeException(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SettingsPropertyWrongTypeException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SettingsPropertyWrongTypeException(string message, Exception innerException)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

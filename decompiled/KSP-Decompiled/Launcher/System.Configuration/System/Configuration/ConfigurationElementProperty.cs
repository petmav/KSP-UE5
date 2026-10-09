using Unity;

namespace System.Configuration;

public sealed class ConfigurationElementProperty
{
	public ConfigurationValidatorBase Validator
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationElementProperty(ConfigurationValidatorBase validator)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

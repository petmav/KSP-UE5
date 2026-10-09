namespace System.Configuration;

public sealed class ConfigurationElementProperty
{
	private ConfigurationValidatorBase validator;

	public ConfigurationValidatorBase Validator => validator;

	public ConfigurationElementProperty(ConfigurationValidatorBase validator)
	{
		this.validator = validator;
	}
}

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public class ConfigurationValidatorAttribute : Attribute
{
	private Type validatorType;

	private ConfigurationValidatorBase instance;

	public virtual ConfigurationValidatorBase ValidatorInstance
	{
		get
		{
			if (instance == null)
			{
				instance = (ConfigurationValidatorBase)Activator.CreateInstance(validatorType);
			}
			return instance;
		}
	}

	public Type ValidatorType => validatorType;

	protected ConfigurationValidatorAttribute()
	{
	}

	public ConfigurationValidatorAttribute(Type validator)
	{
		validatorType = validator;
	}
}

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class PositiveTimeSpanValidatorAttribute : ConfigurationValidatorAttribute
{
	private ConfigurationValidatorBase instance;

	public override ConfigurationValidatorBase ValidatorInstance
	{
		get
		{
			if (instance == null)
			{
				instance = new PositiveTimeSpanValidator();
			}
			return instance;
		}
	}
}

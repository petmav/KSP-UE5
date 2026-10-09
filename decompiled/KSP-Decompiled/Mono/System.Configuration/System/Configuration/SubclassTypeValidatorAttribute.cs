namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class SubclassTypeValidatorAttribute : ConfigurationValidatorAttribute
{
	private Type baseClass;

	private ConfigurationValidatorBase instance;

	public Type BaseClass => baseClass;

	public override ConfigurationValidatorBase ValidatorInstance
	{
		get
		{
			if (instance == null)
			{
				instance = new SubclassTypeValidator(baseClass);
			}
			return instance;
		}
	}

	public SubclassTypeValidatorAttribute(Type baseClass)
	{
		this.baseClass = baseClass;
	}
}

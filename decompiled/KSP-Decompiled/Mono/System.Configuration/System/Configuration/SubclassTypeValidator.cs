namespace System.Configuration;

public sealed class SubclassTypeValidator : ConfigurationValidatorBase
{
	private Type baseClass;

	public SubclassTypeValidator(Type baseClass)
	{
		this.baseClass = baseClass;
	}

	public override bool CanValidate(Type type)
	{
		return type == typeof(Type);
	}

	public override void Validate(object value)
	{
		Type c = (Type)value;
		if (!baseClass.IsAssignableFrom(c))
		{
			throw new ArgumentException("The value must be a subclass");
		}
	}
}

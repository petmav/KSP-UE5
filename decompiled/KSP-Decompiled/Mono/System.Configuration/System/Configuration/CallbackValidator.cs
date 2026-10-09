namespace System.Configuration;

public sealed class CallbackValidator : ConfigurationValidatorBase
{
	private Type type;

	private ValidatorCallback callback;

	public CallbackValidator(Type type, ValidatorCallback callback)
	{
		this.type = type;
		this.callback = callback;
	}

	public override bool CanValidate(Type type)
	{
		return type == this.type;
	}

	public override void Validate(object value)
	{
		callback(value);
	}
}

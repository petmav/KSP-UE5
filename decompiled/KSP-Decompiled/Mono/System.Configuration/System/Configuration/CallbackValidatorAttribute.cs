namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class CallbackValidatorAttribute : ConfigurationValidatorAttribute
{
	private string callbackMethodName = "";

	private Type type;

	private ConfigurationValidatorBase instance;

	public string CallbackMethodName
	{
		get
		{
			return callbackMethodName;
		}
		set
		{
			callbackMethodName = value;
			instance = null;
		}
	}

	public Type Type
	{
		get
		{
			return type;
		}
		set
		{
			type = value;
			instance = null;
		}
	}

	public override ConfigurationValidatorBase ValidatorInstance => instance;
}

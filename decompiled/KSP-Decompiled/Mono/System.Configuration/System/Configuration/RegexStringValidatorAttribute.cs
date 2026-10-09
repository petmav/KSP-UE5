namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class RegexStringValidatorAttribute : ConfigurationValidatorAttribute
{
	private string regex;

	private ConfigurationValidatorBase instance;

	public string Regex => regex;

	public override ConfigurationValidatorBase ValidatorInstance
	{
		get
		{
			if (instance == null)
			{
				instance = new RegexStringValidator(regex);
			}
			return instance;
		}
	}

	public RegexStringValidatorAttribute(string regex)
	{
		this.regex = regex;
	}
}

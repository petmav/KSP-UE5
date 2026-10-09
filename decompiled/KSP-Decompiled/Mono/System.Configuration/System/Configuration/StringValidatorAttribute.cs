namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class StringValidatorAttribute : ConfigurationValidatorAttribute
{
	private string invalidCharacters;

	private int maxLength = int.MaxValue;

	private int minLength;

	private ConfigurationValidatorBase instance;

	public string InvalidCharacters
	{
		get
		{
			return invalidCharacters;
		}
		set
		{
			invalidCharacters = value;
			instance = null;
		}
	}

	public int MaxLength
	{
		get
		{
			return maxLength;
		}
		set
		{
			maxLength = value;
			instance = null;
		}
	}

	public int MinLength
	{
		get
		{
			return minLength;
		}
		set
		{
			minLength = value;
			instance = null;
		}
	}

	public override ConfigurationValidatorBase ValidatorInstance
	{
		get
		{
			if (instance == null)
			{
				instance = new StringValidator(minLength, maxLength, invalidCharacters);
			}
			return instance;
		}
	}
}

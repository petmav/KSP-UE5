namespace System.Configuration;

public class StringValidator : ConfigurationValidatorBase
{
	private char[] invalidCharacters;

	private int maxLength;

	private int minLength;

	public StringValidator(int minLength)
	{
		this.minLength = minLength;
		maxLength = int.MaxValue;
	}

	public StringValidator(int minLength, int maxLength)
	{
		this.minLength = minLength;
		this.maxLength = maxLength;
	}

	public StringValidator(int minLength, int maxLength, string invalidCharacters)
	{
		this.minLength = minLength;
		this.maxLength = maxLength;
		if (invalidCharacters != null)
		{
			this.invalidCharacters = invalidCharacters.ToCharArray();
		}
	}

	public override bool CanValidate(Type type)
	{
		return type == typeof(string);
	}

	public override void Validate(object value)
	{
		if (value != null || minLength > 0)
		{
			string text = (string)value;
			if (text == null || text.Length < minLength)
			{
				throw new ArgumentException("The string must be at least " + minLength + " characters long.");
			}
			if (text.Length > maxLength)
			{
				throw new ArgumentException("The string must be no more than " + maxLength + " characters long.");
			}
			if (invalidCharacters != null && text.IndexOfAny(invalidCharacters) != -1)
			{
				throw new ArgumentException($"The string cannot contain any of the following characters: '{invalidCharacters}'.");
			}
		}
	}
}

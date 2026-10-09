namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class IntegerValidatorAttribute : ConfigurationValidatorAttribute
{
	private bool excludeRange;

	private int maxValue;

	private int minValue;

	private ConfigurationValidatorBase instance;

	public bool ExcludeRange
	{
		get
		{
			return excludeRange;
		}
		set
		{
			excludeRange = value;
			instance = null;
		}
	}

	public int MaxValue
	{
		get
		{
			return maxValue;
		}
		set
		{
			maxValue = value;
			instance = null;
		}
	}

	public int MinValue
	{
		get
		{
			return minValue;
		}
		set
		{
			minValue = value;
			instance = null;
		}
	}

	public override ConfigurationValidatorBase ValidatorInstance
	{
		get
		{
			if (instance == null)
			{
				instance = new IntegerValidator(minValue, maxValue, excludeRange);
			}
			return instance;
		}
	}
}

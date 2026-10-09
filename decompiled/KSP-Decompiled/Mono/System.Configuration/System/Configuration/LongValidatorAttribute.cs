namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class LongValidatorAttribute : ConfigurationValidatorAttribute
{
	private bool excludeRange;

	private long maxValue;

	private long minValue;

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

	public long MaxValue
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

	public long MinValue
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
				instance = new LongValidator(minValue, maxValue, excludeRange);
			}
			return instance;
		}
	}
}

namespace System.Configuration;

public class LongValidator : ConfigurationValidatorBase
{
	private bool rangeIsExclusive;

	private long minValue;

	private long maxValue;

	private long resolution;

	public LongValidator(long minValue, long maxValue, bool rangeIsExclusive, long resolution)
	{
		this.minValue = minValue;
		this.maxValue = maxValue;
		this.rangeIsExclusive = rangeIsExclusive;
		this.resolution = resolution;
	}

	public LongValidator(long minValue, long maxValue, bool rangeIsExclusive)
		: this(minValue, maxValue, rangeIsExclusive, 0L)
	{
	}

	public LongValidator(long minValue, long maxValue)
		: this(minValue, maxValue, rangeIsExclusive: false, 0L)
	{
	}

	public override bool CanValidate(Type type)
	{
		return type == typeof(long);
	}

	public override void Validate(object value)
	{
		long num = (long)value;
		if (!rangeIsExclusive)
		{
			if (num < minValue || num > maxValue)
			{
				throw new ArgumentException("The value must be in the range " + minValue + " - " + maxValue);
			}
		}
		else if (num >= minValue && num <= maxValue)
		{
			throw new ArgumentException("The value must not be in the range " + minValue + " - " + maxValue);
		}
		if (resolution != 0L && num % resolution != 0L)
		{
			throw new ArgumentException("The value must have a resolution of " + resolution);
		}
	}
}

namespace System.Configuration;

public class TimeSpanValidator : ConfigurationValidatorBase
{
	private bool rangeIsExclusive;

	private TimeSpan minValue;

	private TimeSpan maxValue;

	private long resolutionInSeconds;

	public TimeSpanValidator(TimeSpan minValue, TimeSpan maxValue)
		: this(minValue, maxValue, rangeIsExclusive: false, 0L)
	{
	}

	public TimeSpanValidator(TimeSpan minValue, TimeSpan maxValue, bool rangeIsExclusive)
		: this(minValue, maxValue, rangeIsExclusive, 0L)
	{
	}

	public TimeSpanValidator(TimeSpan minValue, TimeSpan maxValue, bool rangeIsExclusive, long resolutionInSeconds)
	{
		this.minValue = minValue;
		this.maxValue = maxValue;
		this.rangeIsExclusive = rangeIsExclusive;
		this.resolutionInSeconds = resolutionInSeconds;
	}

	public override bool CanValidate(Type type)
	{
		return type == typeof(TimeSpan);
	}

	public override void Validate(object value)
	{
		TimeSpan timeSpan = (TimeSpan)value;
		if (!rangeIsExclusive)
		{
			if (timeSpan < minValue || timeSpan > maxValue)
			{
				throw new ArgumentException(string.Concat("The value must be in the range ", minValue, " - ", maxValue));
			}
		}
		else if (timeSpan >= minValue && timeSpan <= maxValue)
		{
			throw new ArgumentException(string.Concat("The value must not be in the range ", minValue, " - ", maxValue));
		}
		if (resolutionInSeconds != 0L && timeSpan.Ticks % (10000000 * resolutionInSeconds) != 0L)
		{
			throw new ArgumentException("The value must have a resolution of " + TimeSpan.FromTicks(10000000 * resolutionInSeconds));
		}
	}
}

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class TimeSpanValidatorAttribute : ConfigurationValidatorAttribute
{
	private bool excludeRange;

	private string maxValueString = "10675199.02:48:05.4775807";

	private string minValueString = "-10675199.02:48:05.4775808";

	public const string TimeSpanMaxValue = "10675199.02:48:05.4775807";

	public const string TimeSpanMinValue = "-10675199.02:48:05.4775808";

	private ConfigurationValidatorBase instance;

	public string MaxValueString
	{
		get
		{
			return maxValueString;
		}
		set
		{
			maxValueString = value;
			instance = null;
		}
	}

	public string MinValueString
	{
		get
		{
			return minValueString;
		}
		set
		{
			minValueString = value;
			instance = null;
		}
	}

	public TimeSpan MaxValue => TimeSpan.Parse(maxValueString);

	public TimeSpan MinValue => TimeSpan.Parse(minValueString);

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

	public override ConfigurationValidatorBase ValidatorInstance
	{
		get
		{
			if (instance == null)
			{
				instance = new TimeSpanValidator(MinValue, MaxValue, excludeRange);
			}
			return instance;
		}
	}
}

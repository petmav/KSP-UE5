using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class TimeSpanValidatorAttribute : ConfigurationValidatorAttribute
{
	public const string TimeSpanMaxValue = "10675199.02:48:05.4775807";

	public const string TimeSpanMinValue = "-10675199.02:48:05.4775808";

	public bool ExcludeRange
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public TimeSpan MaxValue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TimeSpan);
		}
	}

	public string MaxValueString
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public TimeSpan MinValue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TimeSpan);
		}
	}

	public string MinValueString
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public override ConfigurationValidatorBase ValidatorInstance
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}
}

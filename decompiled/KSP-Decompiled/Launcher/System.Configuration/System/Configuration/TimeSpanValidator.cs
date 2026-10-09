using Unity;

namespace System.Configuration;

public class TimeSpanValidator : ConfigurationValidatorBase
{
	public TimeSpanValidator(TimeSpan minValue, TimeSpan maxValue)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TimeSpanValidator(TimeSpan minValue, TimeSpan maxValue, bool rangeIsExclusive)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TimeSpanValidator(TimeSpan minValue, TimeSpan maxValue, bool rangeIsExclusive, long resolutionInSeconds)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override bool CanValidate(Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override void Validate(object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

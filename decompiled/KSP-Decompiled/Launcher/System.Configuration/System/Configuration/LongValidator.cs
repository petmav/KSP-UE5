using Unity;

namespace System.Configuration;

public class LongValidator : ConfigurationValidatorBase
{
	public LongValidator(long minValue, long maxValue)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public LongValidator(long minValue, long maxValue, bool rangeIsExclusive)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public LongValidator(long minValue, long maxValue, bool rangeIsExclusive, long resolution)
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

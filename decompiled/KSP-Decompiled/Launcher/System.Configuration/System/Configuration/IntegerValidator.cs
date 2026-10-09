using Unity;

namespace System.Configuration;

public class IntegerValidator : ConfigurationValidatorBase
{
	public IntegerValidator(int minValue, int maxValue)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IntegerValidator(int minValue, int maxValue, bool rangeIsExclusive)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IntegerValidator(int minValue, int maxValue, bool rangeIsExclusive, int resolution)
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

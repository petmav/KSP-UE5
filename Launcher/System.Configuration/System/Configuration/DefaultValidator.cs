using Unity;

namespace System.Configuration;

public sealed class DefaultValidator : ConfigurationValidatorBase
{
	public DefaultValidator()
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

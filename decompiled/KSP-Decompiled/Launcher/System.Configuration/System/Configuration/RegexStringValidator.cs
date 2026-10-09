using Unity;

namespace System.Configuration;

public class RegexStringValidator : ConfigurationValidatorBase
{
	public RegexStringValidator(string regex)
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

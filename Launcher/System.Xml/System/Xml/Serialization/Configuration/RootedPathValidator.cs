using System.Configuration;
using Unity;

namespace System.Xml.Serialization.Configuration;

public class RootedPathValidator : ConfigurationValidatorBase
{
	public RootedPathValidator()
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

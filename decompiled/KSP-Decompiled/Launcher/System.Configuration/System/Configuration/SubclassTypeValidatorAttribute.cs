using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class SubclassTypeValidatorAttribute : ConfigurationValidatorAttribute
{
	public Type BaseClass
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
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

	public SubclassTypeValidatorAttribute(Type baseClass)
	{
	}
}

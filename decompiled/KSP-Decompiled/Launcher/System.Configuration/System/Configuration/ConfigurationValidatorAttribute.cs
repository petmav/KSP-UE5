using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public class ConfigurationValidatorAttribute : Attribute
{
	public virtual ConfigurationValidatorBase ValidatorInstance
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Type ValidatorType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected ConfigurationValidatorAttribute()
	{
	}

	public ConfigurationValidatorAttribute(Type validator)
	{
	}
}

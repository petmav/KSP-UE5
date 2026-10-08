using Unity;

namespace System.Configuration;

public abstract class ConfigurationValidatorBase
{
	protected ConfigurationValidatorBase()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual bool CanValidate(Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public abstract void Validate(object value);
}

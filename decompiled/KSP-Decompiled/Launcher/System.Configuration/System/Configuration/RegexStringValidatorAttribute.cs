using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class RegexStringValidatorAttribute : ConfigurationValidatorAttribute
{
	public string Regex
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

	public RegexStringValidatorAttribute(string regex)
	{
	}
}

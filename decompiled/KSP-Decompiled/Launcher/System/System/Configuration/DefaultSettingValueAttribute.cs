using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class DefaultSettingValueAttribute : Attribute
{
	public string Value
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public DefaultSettingValueAttribute(string value)
	{
	}
}

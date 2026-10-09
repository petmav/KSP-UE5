namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class DefaultSettingValueAttribute : Attribute
{
	private string value;

	public string Value => value;

	public DefaultSettingValueAttribute(string value)
	{
		this.value = value;
	}
}

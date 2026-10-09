namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class SettingsDescriptionAttribute : Attribute
{
	private string desc;

	public string Description => desc;

	public SettingsDescriptionAttribute(string description)
	{
		desc = description;
	}
}

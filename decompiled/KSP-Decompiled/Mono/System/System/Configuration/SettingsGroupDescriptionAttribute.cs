namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class)]
public sealed class SettingsGroupDescriptionAttribute : Attribute
{
	private string desc;

	public string Description => desc;

	public SettingsGroupDescriptionAttribute(string description)
	{
		desc = description;
	}
}

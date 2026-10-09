namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class)]
public sealed class SettingsGroupNameAttribute : Attribute
{
	private string group_name;

	public string GroupName => group_name;

	public SettingsGroupNameAttribute(string groupName)
	{
		group_name = groupName;
	}
}

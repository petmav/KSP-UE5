namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Property)]
public sealed class SettingsManageabilityAttribute : Attribute
{
	private SettingsManageability manageability;

	public SettingsManageability Manageability => manageability;

	public SettingsManageabilityAttribute(SettingsManageability manageability)
	{
		this.manageability = manageability;
	}
}

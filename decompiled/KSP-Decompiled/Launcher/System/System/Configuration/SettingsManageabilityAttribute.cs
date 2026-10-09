using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Property)]
public sealed class SettingsManageabilityAttribute : Attribute
{
	public SettingsManageability Manageability
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SettingsManageability);
		}
	}

	public SettingsManageabilityAttribute(SettingsManageability manageability)
	{
	}
}

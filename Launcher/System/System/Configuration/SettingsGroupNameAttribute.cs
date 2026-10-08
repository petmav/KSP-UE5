using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class)]
public sealed class SettingsGroupNameAttribute : Attribute
{
	public string GroupName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingsGroupNameAttribute(string groupName)
	{
	}
}

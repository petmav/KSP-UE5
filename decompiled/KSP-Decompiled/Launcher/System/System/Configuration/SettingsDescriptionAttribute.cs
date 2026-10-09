using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class SettingsDescriptionAttribute : Attribute
{
	public string Description
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingsDescriptionAttribute(string description)
	{
	}
}

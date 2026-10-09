using Unity;

namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Property)]
public sealed class SettingsProviderAttribute : Attribute
{
	public string ProviderTypeName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingsProviderAttribute(string providerTypeName)
	{
	}

	public SettingsProviderAttribute(Type providerType)
	{
	}
}

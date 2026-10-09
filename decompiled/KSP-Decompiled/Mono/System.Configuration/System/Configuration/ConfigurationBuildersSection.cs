using Unity;

namespace System.Configuration;

public sealed class ConfigurationBuildersSection : ConfigurationSection
{
	public ProviderSettingsCollection Builders
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationBuildersSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationBuilder GetBuilderFromName(string builderName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

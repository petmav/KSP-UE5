using Unity;

namespace System.Configuration;

public class ConfigurationBuilderSettings : ConfigurationElement
{
	public ProviderSettingsCollection Builders
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected internal override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationBuilderSettings()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

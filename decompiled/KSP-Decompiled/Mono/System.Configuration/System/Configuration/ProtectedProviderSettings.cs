namespace System.Configuration;

public class ProtectedProviderSettings : ConfigurationElement
{
	private static ConfigurationProperty providersProp;

	private static ConfigurationPropertyCollection properties;

	protected internal override ConfigurationPropertyCollection Properties => properties;

	[ConfigurationProperty("", Options = ConfigurationPropertyOptions.IsDefaultCollection)]
	public ProviderSettingsCollection Providers => (ProviderSettingsCollection)base[providersProp];

	static ProtectedProviderSettings()
	{
		providersProp = new ConfigurationProperty("", typeof(ProviderSettingsCollection), null, null, null, ConfigurationPropertyOptions.IsDefaultCollection);
		properties = new ConfigurationPropertyCollection();
		properties.Add(providersProp);
	}
}

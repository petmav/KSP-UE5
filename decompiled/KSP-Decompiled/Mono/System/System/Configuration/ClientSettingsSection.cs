namespace System.Configuration;

public sealed class ClientSettingsSection : ConfigurationSection
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty settings_prop;

	[ConfigurationProperty("", Options = ConfigurationPropertyOptions.IsDefaultCollection)]
	public SettingElementCollection Settings => (SettingElementCollection)base[settings_prop];

	protected override ConfigurationPropertyCollection Properties => properties;

	static ClientSettingsSection()
	{
		settings_prop = new ConfigurationProperty("", typeof(SettingElementCollection), null, ConfigurationPropertyOptions.IsDefaultCollection);
		properties = new ConfigurationPropertyCollection();
		properties.Add(settings_prop);
	}
}

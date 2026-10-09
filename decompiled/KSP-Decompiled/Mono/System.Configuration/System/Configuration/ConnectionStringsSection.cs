namespace System.Configuration;

public sealed class ConnectionStringsSection : ConfigurationSection
{
	private static readonly ConfigurationProperty _propConnectionStrings;

	private static ConfigurationPropertyCollection _properties;

	[ConfigurationProperty("", Options = ConfigurationPropertyOptions.IsDefaultCollection)]
	public ConnectionStringSettingsCollection ConnectionStrings => (ConnectionStringSettingsCollection)base[_propConnectionStrings];

	protected internal override ConfigurationPropertyCollection Properties => _properties;

	static ConnectionStringsSection()
	{
		_propConnectionStrings = new ConfigurationProperty("", typeof(ConnectionStringSettingsCollection), null, ConfigurationPropertyOptions.IsDefaultCollection);
		_properties = new ConfigurationPropertyCollection();
		_properties.Add(_propConnectionStrings);
	}

	protected internal override object GetRuntimeObject()
	{
		return base.GetRuntimeObject();
	}
}

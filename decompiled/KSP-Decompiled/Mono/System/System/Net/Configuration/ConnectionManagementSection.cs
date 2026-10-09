using System.Configuration;

namespace System.Net.Configuration;

public sealed class ConnectionManagementSection : ConfigurationSection
{
	private static ConfigurationProperty connectionManagementProp;

	private static ConfigurationPropertyCollection properties;

	[ConfigurationProperty("", Options = ConfigurationPropertyOptions.IsDefaultCollection)]
	public ConnectionManagementElementCollection ConnectionManagement => (ConnectionManagementElementCollection)base[connectionManagementProp];

	protected override ConfigurationPropertyCollection Properties => properties;

	static ConnectionManagementSection()
	{
		connectionManagementProp = new ConfigurationProperty("ConnectionManagement", typeof(ConnectionManagementElementCollection), null, ConfigurationPropertyOptions.IsDefaultCollection);
		properties = new ConfigurationPropertyCollection();
		properties.Add(connectionManagementProp);
	}
}

using System.Configuration;

namespace System.Net.Configuration;

public sealed class ConnectionManagementElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty addressProp;

	private static ConfigurationProperty maxConnectionProp;

	[ConfigurationProperty("address", Options = (ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey))]
	public string Address
	{
		get
		{
			return (string)base[addressProp];
		}
		set
		{
			base[addressProp] = value;
		}
	}

	[ConfigurationProperty("maxconnection", DefaultValue = "6", Options = ConfigurationPropertyOptions.IsRequired)]
	public int MaxConnection
	{
		get
		{
			return (int)base[maxConnectionProp];
		}
		set
		{
			base[maxConnectionProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static ConnectionManagementElement()
	{
		addressProp = new ConfigurationProperty("address", typeof(string), null, ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey);
		maxConnectionProp = new ConfigurationProperty("maxconnection", typeof(int), 1, ConfigurationPropertyOptions.IsRequired);
		properties = new ConfigurationPropertyCollection();
		properties.Add(addressProp);
		properties.Add(maxConnectionProp);
	}

	public ConnectionManagementElement()
	{
	}

	public ConnectionManagementElement(string address, int maxConnection)
	{
		Address = address;
		MaxConnection = maxConnection;
	}
}

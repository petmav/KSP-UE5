using System.ComponentModel;

namespace System.Configuration;

public sealed class ConnectionStringSettings : ConfigurationElement
{
	private static ConfigurationPropertyCollection _properties;

	private static readonly ConfigurationProperty _propConnectionString;

	private static readonly ConfigurationProperty _propName;

	private static readonly ConfigurationProperty _propProviderName;

	protected internal override ConfigurationPropertyCollection Properties => _properties;

	[ConfigurationProperty("name", DefaultValue = "", Options = (ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey))]
	public string Name
	{
		get
		{
			return (string)base[_propName];
		}
		set
		{
			base[_propName] = value;
		}
	}

	[ConfigurationProperty("providerName", DefaultValue = "System.Data.SqlClient")]
	public string ProviderName
	{
		get
		{
			return (string)base[_propProviderName];
		}
		set
		{
			base[_propProviderName] = value;
		}
	}

	[ConfigurationProperty("connectionString", DefaultValue = "", Options = ConfigurationPropertyOptions.IsRequired)]
	public string ConnectionString
	{
		get
		{
			return (string)base[_propConnectionString];
		}
		set
		{
			base[_propConnectionString] = value;
		}
	}

	static ConnectionStringSettings()
	{
		_properties = new ConfigurationPropertyCollection();
		_propName = new ConfigurationProperty("name", typeof(string), null, TypeDescriptor.GetConverter(typeof(string)), null, ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey);
		_propProviderName = new ConfigurationProperty("providerName", typeof(string), "", ConfigurationPropertyOptions.None);
		_propConnectionString = new ConfigurationProperty("connectionString", typeof(string), "", ConfigurationPropertyOptions.IsRequired);
		_properties.Add(_propName);
		_properties.Add(_propProviderName);
		_properties.Add(_propConnectionString);
	}

	public ConnectionStringSettings()
	{
	}

	public ConnectionStringSettings(string name, string connectionString)
		: this(name, connectionString, "")
	{
	}

	public ConnectionStringSettings(string name, string connectionString, string providerName)
	{
		Name = name;
		ConnectionString = connectionString;
		ProviderName = providerName;
	}

	public override string ToString()
	{
		return ConnectionString;
	}
}

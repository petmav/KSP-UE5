namespace System.Configuration;

public sealed class NameValueConfigurationElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection _properties;

	private static readonly ConfigurationProperty _propName;

	private static readonly ConfigurationProperty _propValue;

	[ConfigurationProperty("name", DefaultValue = "", Options = ConfigurationPropertyOptions.IsKey)]
	public string Name => (string)base[_propName];

	[ConfigurationProperty("value", DefaultValue = "", Options = ConfigurationPropertyOptions.None)]
	public string Value
	{
		get
		{
			return (string)base[_propValue];
		}
		set
		{
			base[_propValue] = value;
		}
	}

	protected internal override ConfigurationPropertyCollection Properties => _properties;

	static NameValueConfigurationElement()
	{
		_properties = new ConfigurationPropertyCollection();
		_propName = new ConfigurationProperty("name", typeof(string), "", ConfigurationPropertyOptions.IsKey);
		_propValue = new ConfigurationProperty("value", typeof(string), "");
		_properties.Add(_propName);
		_properties.Add(_propValue);
	}

	public NameValueConfigurationElement(string name, string value)
	{
		base[_propName] = name;
		base[_propValue] = value;
	}
}

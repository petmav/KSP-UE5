namespace System.Configuration;

public class KeyValueConfigurationElement : ConfigurationElement
{
	private static ConfigurationProperty keyProp;

	private static ConfigurationProperty valueProp;

	private static ConfigurationPropertyCollection properties;

	[ConfigurationProperty("key", DefaultValue = "", Options = ConfigurationPropertyOptions.IsKey)]
	public string Key => (string)base[keyProp];

	[ConfigurationProperty("value", DefaultValue = "")]
	public string Value
	{
		get
		{
			return (string)base[valueProp];
		}
		set
		{
			base[valueProp] = value;
		}
	}

	protected internal override ConfigurationPropertyCollection Properties => properties;

	static KeyValueConfigurationElement()
	{
		keyProp = new ConfigurationProperty("key", typeof(string), "", ConfigurationPropertyOptions.IsKey);
		valueProp = new ConfigurationProperty("value", typeof(string), "");
		properties = new ConfigurationPropertyCollection();
		properties.Add(keyProp);
		properties.Add(valueProp);
	}

	internal KeyValueConfigurationElement()
	{
	}

	public KeyValueConfigurationElement(string key, string value)
	{
		base[keyProp] = key;
		base[valueProp] = value;
	}

	[System.MonoTODO]
	protected internal override void Init()
	{
	}
}

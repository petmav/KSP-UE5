using System.Configuration;

namespace System.Net.Configuration;

public sealed class Ipv6Element : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty enabledProp;

	[ConfigurationProperty("enabled", DefaultValue = "False")]
	public bool Enabled
	{
		get
		{
			return (bool)base[enabledProp];
		}
		set
		{
			base[enabledProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static Ipv6Element()
	{
		enabledProp = new ConfigurationProperty("enabled", typeof(bool), false);
		properties = new ConfigurationPropertyCollection();
		properties.Add(enabledProp);
	}
}

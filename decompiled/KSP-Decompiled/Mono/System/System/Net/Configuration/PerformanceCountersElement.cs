using System.Configuration;

namespace System.Net.Configuration;

public sealed class PerformanceCountersElement : ConfigurationElement
{
	private static ConfigurationProperty enabledProp;

	private static ConfigurationPropertyCollection properties;

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

	static PerformanceCountersElement()
	{
		enabledProp = new ConfigurationProperty("enabled", typeof(bool), false);
		properties = new ConfigurationPropertyCollection();
		properties.Add(enabledProp);
	}
}

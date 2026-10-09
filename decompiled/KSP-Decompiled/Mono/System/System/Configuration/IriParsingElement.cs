namespace System.Configuration;

public sealed class IriParsingElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty enabled_prop;

	[ConfigurationProperty("enabled", DefaultValue = false, Options = (ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey))]
	public bool Enabled
	{
		get
		{
			return (bool)base[enabled_prop];
		}
		set
		{
			base[enabled_prop] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static IriParsingElement()
	{
		enabled_prop = new ConfigurationProperty("enabled", typeof(bool), false, ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey);
		properties = new ConfigurationPropertyCollection();
		properties.Add(enabled_prop);
	}

	public override bool Equals(object o)
	{
		if (!(o is IriParsingElement iriParsingElement))
		{
			return false;
		}
		return iriParsingElement.Enabled == Enabled;
	}

	public override int GetHashCode()
	{
		return Convert.ToInt32(Enabled) ^ 0x7F;
	}
}

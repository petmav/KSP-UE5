namespace System.Configuration;

public sealed class IdnElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty enabled_prop;

	internal const UriIdnScope EnabledDefaultValue = UriIdnScope.None;

	[ConfigurationProperty("enabled", DefaultValue = UriIdnScope.None, Options = (ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey))]
	public UriIdnScope Enabled
	{
		get
		{
			return (UriIdnScope)base[enabled_prop];
		}
		set
		{
			base[enabled_prop] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static IdnElement()
	{
		enabled_prop = new ConfigurationProperty("enabled", typeof(UriIdnScope), UriIdnScope.None, ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey);
		properties = new ConfigurationPropertyCollection();
		properties.Add(enabled_prop);
	}

	public override bool Equals(object o)
	{
		if (!(o is IdnElement idnElement))
		{
			return false;
		}
		return idnElement.Enabled == Enabled;
	}

	public override int GetHashCode()
	{
		return (int)(Enabled ^ (UriIdnScope)127);
	}
}

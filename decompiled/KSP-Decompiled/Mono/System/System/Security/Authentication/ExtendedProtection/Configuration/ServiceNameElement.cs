using System.Configuration;

namespace System.Security.Authentication.ExtendedProtection.Configuration;

public sealed class ServiceNameElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty name;

	[ConfigurationProperty("name")]
	public string Name
	{
		get
		{
			return (string)base[name];
		}
		set
		{
			base[name] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static ServiceNameElement()
	{
		properties = new ConfigurationPropertyCollection();
		name = ConfigUtil.BuildProperty(typeof(ServiceNameElement), "Name");
		properties.Add(name);
	}
}

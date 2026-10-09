using System.Configuration;

namespace System.Net.Configuration;

public sealed class AuthenticationModuleElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty typeProp;

	protected override ConfigurationPropertyCollection Properties => properties;

	[ConfigurationProperty("type", Options = (ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey))]
	public string Type
	{
		get
		{
			return (string)base[typeProp];
		}
		set
		{
			base[typeProp] = value;
		}
	}

	static AuthenticationModuleElement()
	{
		typeProp = new ConfigurationProperty("type", typeof(string), null, ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey);
		properties = new ConfigurationPropertyCollection();
		properties.Add(typeProp);
	}

	public AuthenticationModuleElement()
	{
	}

	public AuthenticationModuleElement(string typeName)
	{
		Type = typeName;
	}
}

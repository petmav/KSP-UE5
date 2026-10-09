using System.Configuration;

namespace System.Net.Configuration;

public sealed class ModuleElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty typeProp;

	protected override ConfigurationPropertyCollection Properties => properties;

	[ConfigurationProperty("type")]
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

	static ModuleElement()
	{
		typeProp = new ConfigurationProperty("type", typeof(string), null);
		properties = new ConfigurationPropertyCollection();
		properties.Add(typeProp);
	}
}

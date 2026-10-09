using System.Configuration;

namespace System.CodeDom.Compiler;

internal sealed class CompilerProviderOption : ConfigurationElement
{
	private static ConfigurationProperty nameProp;

	private static ConfigurationProperty valueProp;

	private static ConfigurationPropertyCollection properties;

	[ConfigurationProperty("name", DefaultValue = "", Options = (ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey))]
	public string Name
	{
		get
		{
			return (string)base[nameProp];
		}
		set
		{
			base[nameProp] = value;
		}
	}

	[ConfigurationProperty("value", DefaultValue = "", Options = ConfigurationPropertyOptions.IsRequired)]
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

	protected override ConfigurationPropertyCollection Properties => properties;

	static CompilerProviderOption()
	{
		nameProp = new ConfigurationProperty("name", typeof(string), "", ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey);
		valueProp = new ConfigurationProperty("value", typeof(string), "", ConfigurationPropertyOptions.IsRequired);
		properties = new ConfigurationPropertyCollection();
		properties.Add(nameProp);
		properties.Add(valueProp);
	}
}

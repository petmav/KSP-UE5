using System.ComponentModel;
using System.Configuration;

namespace System.Net.Configuration;

public sealed class WebRequestModuleElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty prefixProp;

	private static ConfigurationProperty typeProp;

	[ConfigurationProperty("prefix", Options = (ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey))]
	public string Prefix
	{
		get
		{
			return (string)base[prefixProp];
		}
		set
		{
			base[prefixProp] = value;
		}
	}

	[ConfigurationProperty("type")]
	[TypeConverter(typeof(TypeConverter))]
	public Type Type
	{
		get
		{
			return Type.GetType((string)base[typeProp]);
		}
		set
		{
			base[typeProp] = value.FullName;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static WebRequestModuleElement()
	{
		prefixProp = new ConfigurationProperty("prefix", typeof(string), null, ConfigurationPropertyOptions.IsRequired | ConfigurationPropertyOptions.IsKey);
		typeProp = new ConfigurationProperty("type", typeof(string));
		properties = new ConfigurationPropertyCollection();
		properties.Add(prefixProp);
		properties.Add(typeProp);
	}

	public WebRequestModuleElement()
	{
	}

	public WebRequestModuleElement(string prefix, string type)
	{
		base[typeProp] = type;
		Prefix = prefix;
	}

	public WebRequestModuleElement(string prefix, Type type)
		: this(prefix, type.FullName)
	{
	}
}

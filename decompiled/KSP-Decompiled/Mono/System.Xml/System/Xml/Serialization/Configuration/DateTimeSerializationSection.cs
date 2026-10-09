using System.ComponentModel;
using System.Configuration;

namespace System.Xml.Serialization.Configuration;

public sealed class DateTimeSerializationSection : ConfigurationSection
{
	public enum DateTimeSerializationMode
	{
		Default,
		Roundtrip,
		Local
	}

	private ConfigurationPropertyCollection properties = new ConfigurationPropertyCollection();

	private readonly ConfigurationProperty mode = new ConfigurationProperty("mode", typeof(DateTimeSerializationMode), DateTimeSerializationMode.Roundtrip, new EnumConverter(typeof(DateTimeSerializationMode)), null, ConfigurationPropertyOptions.None);

	protected override ConfigurationPropertyCollection Properties => properties;

	[ConfigurationProperty("mode", DefaultValue = DateTimeSerializationMode.Roundtrip)]
	public DateTimeSerializationMode Mode
	{
		get
		{
			return (DateTimeSerializationMode)base[mode];
		}
		set
		{
			base[mode] = value;
		}
	}

	public DateTimeSerializationSection()
	{
		properties.Add(mode);
	}
}

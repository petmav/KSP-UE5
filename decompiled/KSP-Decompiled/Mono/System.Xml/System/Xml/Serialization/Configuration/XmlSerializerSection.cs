using System.Configuration;

namespace System.Xml.Serialization.Configuration;

public sealed class XmlSerializerSection : ConfigurationSection
{
	private ConfigurationPropertyCollection properties = new ConfigurationPropertyCollection();

	private readonly ConfigurationProperty checkDeserializeAdvances = new ConfigurationProperty("checkDeserializeAdvances", typeof(bool), false, ConfigurationPropertyOptions.None);

	private readonly ConfigurationProperty tempFilesLocation = new ConfigurationProperty("tempFilesLocation", typeof(string), null, null, new RootedPathValidator(), ConfigurationPropertyOptions.None);

	private readonly ConfigurationProperty useLegacySerializerGeneration = new ConfigurationProperty("useLegacySerializerGeneration", typeof(bool), false, ConfigurationPropertyOptions.None);

	protected override ConfigurationPropertyCollection Properties => properties;

	[ConfigurationProperty("checkDeserializeAdvances", DefaultValue = false)]
	public bool CheckDeserializeAdvances
	{
		get
		{
			return (bool)base[checkDeserializeAdvances];
		}
		set
		{
			base[checkDeserializeAdvances] = value;
		}
	}

	[ConfigurationProperty("tempFilesLocation", DefaultValue = null)]
	public string TempFilesLocation
	{
		get
		{
			return (string)base[tempFilesLocation];
		}
		set
		{
			base[tempFilesLocation] = value;
		}
	}

	[ConfigurationProperty("useLegacySerializerGeneration", DefaultValue = false)]
	public bool UseLegacySerializerGeneration
	{
		get
		{
			return (bool)base[useLegacySerializerGeneration];
		}
		set
		{
			base[useLegacySerializerGeneration] = value;
		}
	}

	public XmlSerializerSection()
	{
		properties.Add(checkDeserializeAdvances);
		properties.Add(tempFilesLocation);
		properties.Add(useLegacySerializerGeneration);
	}
}

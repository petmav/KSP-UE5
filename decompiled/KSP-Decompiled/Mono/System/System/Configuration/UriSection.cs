using Unity;

namespace System.Configuration;

public sealed class UriSection : ConfigurationSection
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty idn_prop;

	private static ConfigurationProperty iriParsing_prop;

	[ConfigurationProperty("idn")]
	public IdnElement Idn => (IdnElement)base[idn_prop];

	[ConfigurationProperty("iriParsing")]
	public IriParsingElement IriParsing => (IriParsingElement)base[iriParsing_prop];

	protected override ConfigurationPropertyCollection Properties => properties;

	public SchemeSettingElementCollection SchemeSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	static UriSection()
	{
		idn_prop = new ConfigurationProperty("idn", typeof(IdnElement), null);
		iriParsing_prop = new ConfigurationProperty("iriParsing", typeof(IriParsingElement), null);
		properties = new ConfigurationPropertyCollection();
		properties.Add(idn_prop);
		properties.Add(iriParsing_prop);
	}
}

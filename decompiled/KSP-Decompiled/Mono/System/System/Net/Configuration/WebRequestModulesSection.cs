using System.Configuration;

namespace System.Net.Configuration;

public sealed class WebRequestModulesSection : ConfigurationSection
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty webRequestModulesProp;

	protected override ConfigurationPropertyCollection Properties => properties;

	[ConfigurationProperty("", Options = ConfigurationPropertyOptions.IsDefaultCollection)]
	public WebRequestModuleElementCollection WebRequestModules => (WebRequestModuleElementCollection)base[webRequestModulesProp];

	static WebRequestModulesSection()
	{
		webRequestModulesProp = new ConfigurationProperty("", typeof(WebRequestModuleElementCollection), null, ConfigurationPropertyOptions.IsDefaultCollection);
		properties = new ConfigurationPropertyCollection();
		properties.Add(webRequestModulesProp);
	}

	[System.MonoTODO]
	protected override void PostDeserialize()
	{
	}

	[System.MonoTODO]
	protected override void InitializeDefault()
	{
	}
}

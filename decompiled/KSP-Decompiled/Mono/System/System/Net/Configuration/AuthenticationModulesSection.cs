using System.Configuration;

namespace System.Net.Configuration;

public sealed class AuthenticationModulesSection : ConfigurationSection
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty authenticationModulesProp;

	protected override ConfigurationPropertyCollection Properties => properties;

	[ConfigurationProperty("", Options = ConfigurationPropertyOptions.IsDefaultCollection)]
	public AuthenticationModuleElementCollection AuthenticationModules => (AuthenticationModuleElementCollection)base[authenticationModulesProp];

	static AuthenticationModulesSection()
	{
		authenticationModulesProp = new ConfigurationProperty("", typeof(AuthenticationModuleElementCollection), null, ConfigurationPropertyOptions.IsDefaultCollection);
		properties = new ConfigurationPropertyCollection();
		properties.Add(authenticationModulesProp);
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

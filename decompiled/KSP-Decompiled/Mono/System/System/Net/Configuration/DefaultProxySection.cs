using System.Configuration;

namespace System.Net.Configuration;

public sealed class DefaultProxySection : ConfigurationSection
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty bypassListProp;

	private static ConfigurationProperty enabledProp;

	private static ConfigurationProperty moduleProp;

	private static ConfigurationProperty proxyProp;

	private static ConfigurationProperty useDefaultCredentialsProp;

	[ConfigurationProperty("bypasslist")]
	public BypassElementCollection BypassList => (BypassElementCollection)base[bypassListProp];

	[ConfigurationProperty("enabled", DefaultValue = "True")]
	public bool Enabled
	{
		get
		{
			return (bool)base[enabledProp];
		}
		set
		{
			base[enabledProp] = value;
		}
	}

	[ConfigurationProperty("module")]
	public ModuleElement Module => (ModuleElement)base[moduleProp];

	[ConfigurationProperty("proxy")]
	public ProxyElement Proxy => (ProxyElement)base[proxyProp];

	[ConfigurationProperty("useDefaultCredentials", DefaultValue = "False")]
	public bool UseDefaultCredentials
	{
		get
		{
			return (bool)base[useDefaultCredentialsProp];
		}
		set
		{
			base[useDefaultCredentialsProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static DefaultProxySection()
	{
		bypassListProp = new ConfigurationProperty("bypasslist", typeof(BypassElementCollection), null);
		enabledProp = new ConfigurationProperty("enabled", typeof(bool), true);
		moduleProp = new ConfigurationProperty("module", typeof(ModuleElement), null);
		proxyProp = new ConfigurationProperty("proxy", typeof(ProxyElement), null);
		useDefaultCredentialsProp = new ConfigurationProperty("useDefaultCredentials", typeof(bool), false);
		properties = new ConfigurationPropertyCollection();
		properties.Add(bypassListProp);
		properties.Add(enabledProp);
		properties.Add(moduleProp);
		properties.Add(proxyProp);
		properties.Add(useDefaultCredentialsProp);
	}

	[System.MonoTODO]
	protected override void PostDeserialize()
	{
	}

	[System.MonoTODO]
	protected override void Reset(ConfigurationElement parentElement)
	{
	}
}

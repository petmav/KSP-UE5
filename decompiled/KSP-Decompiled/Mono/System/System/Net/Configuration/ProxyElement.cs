using System.Configuration;

namespace System.Net.Configuration;

public sealed class ProxyElement : ConfigurationElement
{
	public enum BypassOnLocalValues
	{
		Unspecified = -1,
		True = 1,
		False = 0
	}

	public enum UseSystemDefaultValues
	{
		Unspecified = -1,
		True = 1,
		False = 0
	}

	public enum AutoDetectValues
	{
		Unspecified = -1,
		True = 1,
		False = 0
	}

	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty autoDetectProp;

	private static ConfigurationProperty bypassOnLocalProp;

	private static ConfigurationProperty proxyAddressProp;

	private static ConfigurationProperty scriptLocationProp;

	private static ConfigurationProperty useSystemDefaultProp;

	[ConfigurationProperty("autoDetect", DefaultValue = "Unspecified")]
	public AutoDetectValues AutoDetect
	{
		get
		{
			return (AutoDetectValues)base[autoDetectProp];
		}
		set
		{
			base[autoDetectProp] = value;
		}
	}

	[ConfigurationProperty("bypassonlocal", DefaultValue = "Unspecified")]
	public BypassOnLocalValues BypassOnLocal
	{
		get
		{
			return (BypassOnLocalValues)base[bypassOnLocalProp];
		}
		set
		{
			base[bypassOnLocalProp] = value;
		}
	}

	[ConfigurationProperty("proxyaddress")]
	public Uri ProxyAddress
	{
		get
		{
			return (Uri)base[proxyAddressProp];
		}
		set
		{
			base[proxyAddressProp] = value;
		}
	}

	[ConfigurationProperty("scriptLocation")]
	public Uri ScriptLocation
	{
		get
		{
			return (Uri)base[scriptLocationProp];
		}
		set
		{
			base[scriptLocationProp] = value;
		}
	}

	[ConfigurationProperty("usesystemdefault", DefaultValue = "Unspecified")]
	public UseSystemDefaultValues UseSystemDefault
	{
		get
		{
			return (UseSystemDefaultValues)base[useSystemDefaultProp];
		}
		set
		{
			base[useSystemDefaultProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static ProxyElement()
	{
		autoDetectProp = new ConfigurationProperty("autoDetect", typeof(AutoDetectValues), AutoDetectValues.Unspecified);
		bypassOnLocalProp = new ConfigurationProperty("bypassonlocal", typeof(BypassOnLocalValues), BypassOnLocalValues.Unspecified);
		proxyAddressProp = new ConfigurationProperty("proxyaddress", typeof(Uri), null);
		scriptLocationProp = new ConfigurationProperty("scriptLocation", typeof(Uri), null);
		useSystemDefaultProp = new ConfigurationProperty("usesystemdefault", typeof(UseSystemDefaultValues), UseSystemDefaultValues.Unspecified);
		properties = new ConfigurationPropertyCollection();
		properties.Add(autoDetectProp);
		properties.Add(bypassOnLocalProp);
		properties.Add(proxyAddressProp);
		properties.Add(scriptLocationProp);
		properties.Add(useSystemDefaultProp);
	}
}

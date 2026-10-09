using System.Configuration;
using System.Net.Cache;
using System.Xml;

namespace System.Net.Configuration;

public sealed class HttpCachePolicyElement : ConfigurationElement
{
	private static ConfigurationProperty maximumAgeProp;

	private static ConfigurationProperty maximumStaleProp;

	private static ConfigurationProperty minimumFreshProp;

	private static ConfigurationProperty policyLevelProp;

	private static ConfigurationPropertyCollection properties;

	[ConfigurationProperty("maximumAge", DefaultValue = "10675199.02:48:05.4775807")]
	public TimeSpan MaximumAge
	{
		get
		{
			return (TimeSpan)base[maximumAgeProp];
		}
		set
		{
			base[maximumAgeProp] = value;
		}
	}

	[ConfigurationProperty("maximumStale", DefaultValue = "-10675199.02:48:05.4775808")]
	public TimeSpan MaximumStale
	{
		get
		{
			return (TimeSpan)base[maximumStaleProp];
		}
		set
		{
			base[maximumStaleProp] = value;
		}
	}

	[ConfigurationProperty("minimumFresh", DefaultValue = "-10675199.02:48:05.4775808")]
	public TimeSpan MinimumFresh
	{
		get
		{
			return (TimeSpan)base[minimumFreshProp];
		}
		set
		{
			base[minimumFreshProp] = value;
		}
	}

	[ConfigurationProperty("policyLevel", DefaultValue = "Default", Options = ConfigurationPropertyOptions.IsRequired)]
	public HttpRequestCacheLevel PolicyLevel
	{
		get
		{
			return (HttpRequestCacheLevel)base[policyLevelProp];
		}
		set
		{
			base[policyLevelProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static HttpCachePolicyElement()
	{
		maximumAgeProp = new ConfigurationProperty("maximumAge", typeof(TimeSpan), TimeSpan.MaxValue);
		maximumStaleProp = new ConfigurationProperty("maximumStale", typeof(TimeSpan), TimeSpan.MinValue);
		minimumFreshProp = new ConfigurationProperty("minimumFresh", typeof(TimeSpan), TimeSpan.MinValue);
		policyLevelProp = new ConfigurationProperty("policyLevel", typeof(HttpRequestCacheLevel), HttpRequestCacheLevel.Default, ConfigurationPropertyOptions.IsRequired);
		properties = new ConfigurationPropertyCollection();
		properties.Add(maximumAgeProp);
		properties.Add(maximumStaleProp);
		properties.Add(minimumFreshProp);
		properties.Add(policyLevelProp);
	}

	[System.MonoTODO]
	protected override void DeserializeElement(XmlReader reader, bool serializeCollectionKey)
	{
		throw new NotImplementedException();
	}

	[System.MonoTODO]
	protected override void Reset(ConfigurationElement parentElement)
	{
		throw new NotImplementedException();
	}
}

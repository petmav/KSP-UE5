using System.Configuration;
using System.Net.Cache;
using System.Xml;

namespace System.Net.Configuration;

public sealed class RequestCachingSection : ConfigurationSection
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty defaultFtpCachePolicyProp;

	private static ConfigurationProperty defaultHttpCachePolicyProp;

	private static ConfigurationProperty defaultPolicyLevelProp;

	private static ConfigurationProperty disableAllCachingProp;

	private static ConfigurationProperty isPrivateCacheProp;

	private static ConfigurationProperty unspecifiedMaximumAgeProp;

	[ConfigurationProperty("defaultFtpCachePolicy")]
	public FtpCachePolicyElement DefaultFtpCachePolicy => (FtpCachePolicyElement)base[defaultFtpCachePolicyProp];

	[ConfigurationProperty("defaultHttpCachePolicy")]
	public HttpCachePolicyElement DefaultHttpCachePolicy => (HttpCachePolicyElement)base[defaultHttpCachePolicyProp];

	[ConfigurationProperty("defaultPolicyLevel", DefaultValue = "BypassCache")]
	public RequestCacheLevel DefaultPolicyLevel
	{
		get
		{
			return (RequestCacheLevel)base[defaultPolicyLevelProp];
		}
		set
		{
			base[defaultPolicyLevelProp] = value;
		}
	}

	[ConfigurationProperty("disableAllCaching", DefaultValue = "False")]
	public bool DisableAllCaching
	{
		get
		{
			return (bool)base[disableAllCachingProp];
		}
		set
		{
			base[disableAllCachingProp] = value;
		}
	}

	[ConfigurationProperty("isPrivateCache", DefaultValue = "True")]
	public bool IsPrivateCache
	{
		get
		{
			return (bool)base[isPrivateCacheProp];
		}
		set
		{
			base[isPrivateCacheProp] = value;
		}
	}

	[ConfigurationProperty("unspecifiedMaximumAge", DefaultValue = "1.00:00:00")]
	public TimeSpan UnspecifiedMaximumAge
	{
		get
		{
			return (TimeSpan)base[unspecifiedMaximumAgeProp];
		}
		set
		{
			base[unspecifiedMaximumAgeProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static RequestCachingSection()
	{
		defaultFtpCachePolicyProp = new ConfigurationProperty("defaultFtpCachePolicy", typeof(FtpCachePolicyElement));
		defaultHttpCachePolicyProp = new ConfigurationProperty("defaultHttpCachePolicy", typeof(HttpCachePolicyElement));
		defaultPolicyLevelProp = new ConfigurationProperty("defaultPolicyLevel", typeof(RequestCacheLevel), RequestCacheLevel.BypassCache);
		disableAllCachingProp = new ConfigurationProperty("disableAllCaching", typeof(bool), false);
		isPrivateCacheProp = new ConfigurationProperty("isPrivateCache", typeof(bool), true);
		unspecifiedMaximumAgeProp = new ConfigurationProperty("unspecifiedMaximumAge", typeof(TimeSpan), new TimeSpan(1, 0, 0, 0));
		properties = new ConfigurationPropertyCollection();
		properties.Add(defaultFtpCachePolicyProp);
		properties.Add(defaultHttpCachePolicyProp);
		properties.Add(defaultPolicyLevelProp);
		properties.Add(disableAllCachingProp);
		properties.Add(isPrivateCacheProp);
		properties.Add(unspecifiedMaximumAgeProp);
	}

	[System.MonoTODO]
	protected override void PostDeserialize()
	{
		base.PostDeserialize();
	}

	[System.MonoTODO]
	protected override void DeserializeElement(XmlReader reader, bool serializeCollectionKey)
	{
		base.DeserializeElement(reader, serializeCollectionKey);
	}
}

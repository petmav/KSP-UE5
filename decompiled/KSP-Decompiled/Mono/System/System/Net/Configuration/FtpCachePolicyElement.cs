using System.Configuration;
using System.Net.Cache;
using System.Xml;

namespace System.Net.Configuration;

public sealed class FtpCachePolicyElement : ConfigurationElement
{
	private static ConfigurationProperty policyLevelProp;

	private static ConfigurationPropertyCollection properties;

	[ConfigurationProperty("policyLevel", DefaultValue = "Default")]
	public RequestCacheLevel PolicyLevel
	{
		get
		{
			return (RequestCacheLevel)base[policyLevelProp];
		}
		set
		{
			base[policyLevelProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	static FtpCachePolicyElement()
	{
		policyLevelProp = new ConfigurationProperty("policyLevel", typeof(RequestCacheLevel), RequestCacheLevel.Default);
		properties = new ConfigurationPropertyCollection();
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

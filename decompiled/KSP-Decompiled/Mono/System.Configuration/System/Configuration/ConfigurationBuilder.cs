using System.Configuration.Provider;
using System.Xml;
using Unity;

namespace System.Configuration;

public abstract class ConfigurationBuilder : ProviderBase
{
	protected ConfigurationBuilder()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual ConfigurationSection ProcessConfigurationSection(ConfigurationSection configSection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual XmlNode ProcessRawXml(XmlNode rawXml)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

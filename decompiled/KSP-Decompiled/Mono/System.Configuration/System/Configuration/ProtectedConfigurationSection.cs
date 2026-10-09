using System.Xml;

namespace System.Configuration;

public sealed class ProtectedConfigurationSection : ConfigurationSection
{
	private static ConfigurationProperty defaultProviderProp;

	private static ConfigurationProperty providersProp;

	private static ConfigurationPropertyCollection properties;

	private ProtectedConfigurationProviderCollection providers;

	[ConfigurationProperty("defaultProvider", DefaultValue = "RsaProtectedConfigurationProvider")]
	public string DefaultProvider
	{
		get
		{
			return (string)base[defaultProviderProp];
		}
		set
		{
			base[defaultProviderProp] = value;
		}
	}

	[ConfigurationProperty("providers")]
	public ProviderSettingsCollection Providers => (ProviderSettingsCollection)base[providersProp];

	protected internal override ConfigurationPropertyCollection Properties => properties;

	static ProtectedConfigurationSection()
	{
		defaultProviderProp = new ConfigurationProperty("defaultProvider", typeof(string), "RsaProtectedConfigurationProvider");
		providersProp = new ConfigurationProperty("providers", typeof(ProviderSettingsCollection), null);
		properties = new ConfigurationPropertyCollection();
		properties.Add(defaultProviderProp);
		properties.Add(providersProp);
	}

	internal string EncryptSection(string clearXml, ProtectedConfigurationProvider protectionProvider)
	{
		XmlDocument xmlDocument = new ConfigurationXmlDocument();
		xmlDocument.LoadXml(clearXml);
		return protectionProvider.Encrypt(xmlDocument.DocumentElement).OuterXml;
	}

	internal string DecryptSection(string encryptedXml, ProtectedConfigurationProvider protectionProvider)
	{
		XmlDocument xmlDocument = new ConfigurationXmlDocument();
		xmlDocument.InnerXml = encryptedXml;
		return protectionProvider.Decrypt(xmlDocument.DocumentElement).OuterXml;
	}

	internal ProtectedConfigurationProviderCollection GetAllProviders()
	{
		if (providers == null)
		{
			providers = new ProtectedConfigurationProviderCollection();
			foreach (ProviderSettings provider in Providers)
			{
				providers.Add(InstantiateProvider(provider));
			}
		}
		return providers;
	}

	private ProtectedConfigurationProvider InstantiateProvider(ProviderSettings ps)
	{
		ProtectedConfigurationProvider obj = (Activator.CreateInstance(Type.GetType(ps.Type, throwOnError: true)) as ProtectedConfigurationProvider) ?? throw new Exception("The type specified does not extend ProtectedConfigurationProvider class.");
		obj.Initialize(ps.Name, ps.Parameters);
		return obj;
	}
}

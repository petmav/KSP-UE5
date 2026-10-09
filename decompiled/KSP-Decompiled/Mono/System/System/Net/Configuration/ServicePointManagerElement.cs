using System.Configuration;
using System.Net.Security;
using Unity;

namespace System.Net.Configuration;

public sealed class ServicePointManagerElement : ConfigurationElement
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty checkCertificateNameProp;

	private static ConfigurationProperty checkCertificateRevocationListProp;

	private static ConfigurationProperty dnsRefreshTimeoutProp;

	private static ConfigurationProperty enableDnsRoundRobinProp;

	private static ConfigurationProperty expect100ContinueProp;

	private static ConfigurationProperty useNagleAlgorithmProp;

	[ConfigurationProperty("checkCertificateName", DefaultValue = "True")]
	public bool CheckCertificateName
	{
		get
		{
			return (bool)base[checkCertificateNameProp];
		}
		set
		{
			base[checkCertificateNameProp] = value;
		}
	}

	[ConfigurationProperty("checkCertificateRevocationList", DefaultValue = "False")]
	public bool CheckCertificateRevocationList
	{
		get
		{
			return (bool)base[checkCertificateRevocationListProp];
		}
		set
		{
			base[checkCertificateRevocationListProp] = value;
		}
	}

	[ConfigurationProperty("dnsRefreshTimeout", DefaultValue = "120000")]
	public int DnsRefreshTimeout
	{
		get
		{
			return (int)base[dnsRefreshTimeoutProp];
		}
		set
		{
			base[dnsRefreshTimeoutProp] = value;
		}
	}

	[ConfigurationProperty("enableDnsRoundRobin", DefaultValue = "False")]
	public bool EnableDnsRoundRobin
	{
		get
		{
			return (bool)base[enableDnsRoundRobinProp];
		}
		set
		{
			base[enableDnsRoundRobinProp] = value;
		}
	}

	[ConfigurationProperty("expect100Continue", DefaultValue = "True")]
	public bool Expect100Continue
	{
		get
		{
			return (bool)base[expect100ContinueProp];
		}
		set
		{
			base[expect100ContinueProp] = value;
		}
	}

	[ConfigurationProperty("useNagleAlgorithm", DefaultValue = "True")]
	public bool UseNagleAlgorithm
	{
		get
		{
			return (bool)base[useNagleAlgorithmProp];
		}
		set
		{
			base[useNagleAlgorithmProp] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	public EncryptionPolicy EncryptionPolicy
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(EncryptionPolicy);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	static ServicePointManagerElement()
	{
		checkCertificateNameProp = new ConfigurationProperty("checkCertificateName", typeof(bool), true);
		checkCertificateRevocationListProp = new ConfigurationProperty("checkCertificateRevocationList", typeof(bool), false);
		dnsRefreshTimeoutProp = new ConfigurationProperty("dnsRefreshTimeout", typeof(int), 120000);
		enableDnsRoundRobinProp = new ConfigurationProperty("enableDnsRoundRobin", typeof(bool), false);
		expect100ContinueProp = new ConfigurationProperty("expect100Continue", typeof(bool), true);
		useNagleAlgorithmProp = new ConfigurationProperty("useNagleAlgorithm", typeof(bool), true);
		properties = new ConfigurationPropertyCollection();
		properties.Add(checkCertificateNameProp);
		properties.Add(checkCertificateRevocationListProp);
		properties.Add(dnsRefreshTimeoutProp);
		properties.Add(enableDnsRoundRobinProp);
		properties.Add(expect100ContinueProp);
		properties.Add(useNagleAlgorithmProp);
	}

	[System.MonoTODO]
	protected override void PostDeserialize()
	{
	}
}

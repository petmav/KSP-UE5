using System.Configuration;
using System.Net.Security;
using Unity;

namespace System.Net.Configuration;

public sealed class ServicePointManagerElement : ConfigurationElement
{
	public bool CheckCertificateName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool CheckCertificateRevocationList
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int DnsRefreshTimeout
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool EnableDnsRoundRobin
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

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

	public bool Expect100Continue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool UseNagleAlgorithm
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ServicePointManagerElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void PostDeserialize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

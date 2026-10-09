using Unity;

namespace System.Configuration;

public sealed class ConnectionStringSettings : ConfigurationElement
{
	public string ConnectionString
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string Name
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected internal override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string ProviderName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ConnectionStringSettings()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConnectionStringSettings(string name, string connectionString)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConnectionStringSettings(string name, string connectionString, string providerName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

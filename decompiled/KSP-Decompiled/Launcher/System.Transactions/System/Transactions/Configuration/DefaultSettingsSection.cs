using System.Configuration;
using Unity;

namespace System.Transactions.Configuration;

public sealed class DefaultSettingsSection : ConfigurationSection
{
	public string DistributedTransactionManagerName
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

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public TimeSpan Timeout
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TimeSpan);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public DefaultSettingsSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Configuration;
using Unity;

namespace System.Transactions.Configuration;

public sealed class TransactionsSectionGroup : ConfigurationSectionGroup
{
	public DefaultSettingsSection DefaultSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public MachineSettingsSection MachineSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public TransactionsSectionGroup()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static TransactionsSectionGroup GetSectionGroup(System.Configuration.Configuration config)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

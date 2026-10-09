using System.Configuration;
using Unity;

namespace System.Transactions.Configuration;

public sealed class MachineSettingsSection : ConfigurationSection
{
	public TimeSpan MaxTimeout
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

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public MachineSettingsSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

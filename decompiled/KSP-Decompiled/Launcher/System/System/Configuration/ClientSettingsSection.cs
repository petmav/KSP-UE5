using Unity;

namespace System.Configuration;

public sealed class ClientSettingsSection : ConfigurationSection
{
	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingElementCollection Settings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ClientSettingsSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

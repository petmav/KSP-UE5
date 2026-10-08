using Unity;

namespace System.Configuration;

public sealed class ProtectedConfigurationSection : ConfigurationSection
{
	public string DefaultProvider
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

	public ProviderSettingsCollection Providers
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ProtectedConfigurationSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

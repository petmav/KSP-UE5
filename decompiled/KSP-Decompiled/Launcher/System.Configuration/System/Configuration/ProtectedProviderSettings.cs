using Unity;

namespace System.Configuration;

public class ProtectedProviderSettings : ConfigurationElement
{
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

	public ProtectedProviderSettings()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

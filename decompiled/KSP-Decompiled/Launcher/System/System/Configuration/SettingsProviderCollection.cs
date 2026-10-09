using System.Configuration.Provider;
using Unity;

namespace System.Configuration;

public class SettingsProviderCollection : ProviderCollection
{
	public new SettingsProvider this[string name]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingsProviderCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Add(ProviderBase provider)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

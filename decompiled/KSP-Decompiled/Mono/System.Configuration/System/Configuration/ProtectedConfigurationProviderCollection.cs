using System.Configuration.Provider;

namespace System.Configuration;

public class ProtectedConfigurationProviderCollection : ProviderCollection
{
	[System.MonoTODO]
	public new ProtectedConfigurationProvider this[string name] => (ProtectedConfigurationProvider)base[name];

	[System.MonoTODO]
	public override void Add(ProviderBase provider)
	{
		base.Add(provider);
	}
}

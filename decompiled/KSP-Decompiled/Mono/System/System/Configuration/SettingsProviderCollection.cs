using System.Configuration.Provider;

namespace System.Configuration;

public class SettingsProviderCollection : ProviderCollection
{
	public new SettingsProvider this[string name] => (SettingsProvider)base[name];

	public override void Add(ProviderBase provider)
	{
		if (!(provider is SettingsProvider))
		{
			throw new ArgumentException("SettingsProvider is expected");
		}
		if (string.IsNullOrEmpty(provider.Name))
		{
			throw new ArgumentException("Provider name cannot be null or empty");
		}
		base.Add(provider);
	}
}

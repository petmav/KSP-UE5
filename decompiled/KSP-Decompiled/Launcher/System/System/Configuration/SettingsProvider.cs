using System.Configuration.Provider;
using Unity;

namespace System.Configuration;

public abstract class SettingsProvider : ProviderBase
{
	public abstract string ApplicationName { get; set; }

	protected SettingsProvider()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public abstract SettingsPropertyValueCollection GetPropertyValues(SettingsContext context, SettingsPropertyCollection collection);

	public abstract void SetPropertyValues(SettingsContext context, SettingsPropertyValueCollection collection);
}

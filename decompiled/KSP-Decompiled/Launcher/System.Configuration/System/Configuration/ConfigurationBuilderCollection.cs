using System.Configuration.Provider;
using Unity;

namespace System.Configuration;

public class ConfigurationBuilderCollection : ProviderCollection
{
	public new ConfigurationBuilder this[string name]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ConfigurationBuilderCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Add(ProviderBase builder)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

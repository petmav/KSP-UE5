using System.Configuration.Provider;
using Unity;

namespace System.Configuration;

public class ProtectedConfigurationProviderCollection : ProviderCollection
{
	public new ProtectedConfigurationProvider this[string name]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ProtectedConfigurationProviderCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Add(ProviderBase provider)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

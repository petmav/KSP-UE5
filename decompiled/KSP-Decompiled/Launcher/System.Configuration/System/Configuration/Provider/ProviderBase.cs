using System.Collections.Specialized;
using Unity;

namespace System.Configuration.Provider;

public abstract class ProviderBase
{
	public virtual string Description
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public virtual string Name
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected ProviderBase()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void Initialize(string name, NameValueCollection config)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

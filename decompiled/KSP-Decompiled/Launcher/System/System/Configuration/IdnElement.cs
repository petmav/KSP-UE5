using Unity;

namespace System.Configuration;

public sealed class IdnElement : ConfigurationElement
{
	public UriIdnScope Enabled
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(UriIdnScope);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public IdnElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

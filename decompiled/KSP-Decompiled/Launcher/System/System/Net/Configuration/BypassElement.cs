using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class BypassElement : ConfigurationElement
{
	public string Address
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

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public BypassElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public BypassElement(string address)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

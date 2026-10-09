using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class WebRequestModuleElement : ConfigurationElement
{
	public string Prefix
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

	public Type Type
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

	public WebRequestModuleElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public WebRequestModuleElement(string prefix, string type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public WebRequestModuleElement(string prefix, Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

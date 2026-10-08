using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class HttpListenerElement : ConfigurationElement
{
	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public HttpListenerTimeoutsElement Timeouts
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool UnescapeRequestUrl
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public HttpListenerElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

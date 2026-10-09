using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class HttpListenerElement : ConfigurationElement
{
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

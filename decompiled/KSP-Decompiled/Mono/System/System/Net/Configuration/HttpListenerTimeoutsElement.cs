using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class HttpListenerTimeoutsElement : ConfigurationElement
{
	public TimeSpan DrainEntityBody
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TimeSpan);
		}
	}

	public TimeSpan EntityBody
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TimeSpan);
		}
	}

	public TimeSpan HeaderWait
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TimeSpan);
		}
	}

	public TimeSpan IdleConnection
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TimeSpan);
		}
	}

	public long MinSendBytesPerSecond
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public TimeSpan RequestQueue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TimeSpan);
		}
	}

	public HttpListenerTimeoutsElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

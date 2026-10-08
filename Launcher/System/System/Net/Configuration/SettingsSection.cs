using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class SettingsSection : ConfigurationSection
{
	public HttpListenerElement HttpListener
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public HttpWebRequestElement HttpWebRequest
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Ipv6Element Ipv6
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PerformanceCountersElement PerformanceCounters
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
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

	public ServicePointManagerElement ServicePointManager
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SocketElement Socket
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public WebProxyScriptElement WebProxyScript
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public WebUtilityElement WebUtility
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingsSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

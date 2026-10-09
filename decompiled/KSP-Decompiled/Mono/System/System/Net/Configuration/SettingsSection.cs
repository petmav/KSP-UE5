using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class SettingsSection : ConfigurationSection
{
	private static ConfigurationPropertyCollection properties;

	private static ConfigurationProperty httpWebRequestProp;

	private static ConfigurationProperty ipv6Prop;

	private static ConfigurationProperty performanceCountersProp;

	private static ConfigurationProperty servicePointManagerProp;

	private static ConfigurationProperty webProxyScriptProp;

	private static ConfigurationProperty socketProp;

	[ConfigurationProperty("httpWebRequest")]
	public HttpWebRequestElement HttpWebRequest => (HttpWebRequestElement)base[httpWebRequestProp];

	[ConfigurationProperty("ipv6")]
	public Ipv6Element Ipv6 => (Ipv6Element)base[ipv6Prop];

	[ConfigurationProperty("performanceCounters")]
	public PerformanceCountersElement PerformanceCounters => (PerformanceCountersElement)base[performanceCountersProp];

	[ConfigurationProperty("servicePointManager")]
	public ServicePointManagerElement ServicePointManager => (ServicePointManagerElement)base[servicePointManagerProp];

	[ConfigurationProperty("socket")]
	public SocketElement Socket => (SocketElement)base[socketProp];

	[ConfigurationProperty("webProxyScript")]
	public WebProxyScriptElement WebProxyScript => (WebProxyScriptElement)base[webProxyScriptProp];

	protected override ConfigurationPropertyCollection Properties => properties;

	public HttpListenerElement HttpListener
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

	static SettingsSection()
	{
		httpWebRequestProp = new ConfigurationProperty("httpWebRequest", typeof(HttpWebRequestElement));
		ipv6Prop = new ConfigurationProperty("ipv6", typeof(Ipv6Element));
		performanceCountersProp = new ConfigurationProperty("performanceCounters", typeof(PerformanceCountersElement));
		servicePointManagerProp = new ConfigurationProperty("servicePointManager", typeof(ServicePointManagerElement));
		socketProp = new ConfigurationProperty("socket", typeof(SocketElement));
		webProxyScriptProp = new ConfigurationProperty("webProxyScript", typeof(WebProxyScriptElement));
		properties = new ConfigurationPropertyCollection();
		properties.Add(httpWebRequestProp);
		properties.Add(ipv6Prop);
		properties.Add(performanceCountersProp);
		properties.Add(servicePointManagerProp);
		properties.Add(socketProp);
		properties.Add(webProxyScriptProp);
	}
}

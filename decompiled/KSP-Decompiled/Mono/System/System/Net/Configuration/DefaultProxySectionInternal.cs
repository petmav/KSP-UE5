using System.Configuration;
using System.Threading;

namespace System.Net.Configuration;

internal sealed class DefaultProxySectionInternal
{
	private IWebProxy webProxy;

	private static object classSyncObject;

	internal static object ClassSyncObject
	{
		get
		{
			if (classSyncObject == null)
			{
				object value = new object();
				Interlocked.CompareExchange(ref classSyncObject, value, null);
			}
			return classSyncObject;
		}
	}

	internal IWebProxy WebProxy => webProxy;

	private static IWebProxy GetDefaultProxy_UsingOldMonoCode()
	{
		if (!(ConfigurationManager.GetSection("system.net/defaultProxy") is DefaultProxySection { Proxy: var proxy } defaultProxySection))
		{
			return GetSystemWebProxy();
		}
		WebProxy webProxy;
		if (proxy.UseSystemDefault != 0 && proxy.ProxyAddress == null)
		{
			IWebProxy systemWebProxy = GetSystemWebProxy();
			if (!(systemWebProxy is WebProxy))
			{
				return systemWebProxy;
			}
			webProxy = (WebProxy)systemWebProxy;
		}
		else
		{
			webProxy = new WebProxy();
		}
		if (proxy.ProxyAddress != null)
		{
			webProxy.Address = proxy.ProxyAddress;
		}
		if (proxy.BypassOnLocal != ProxyElement.BypassOnLocalValues.Unspecified)
		{
			webProxy.BypassProxyOnLocal = proxy.BypassOnLocal == ProxyElement.BypassOnLocalValues.True;
		}
		foreach (BypassElement bypass in defaultProxySection.BypassList)
		{
			webProxy.BypassArrayList.Add(bypass.Address);
		}
		return webProxy;
	}

	private static IWebProxy GetSystemWebProxy()
	{
		return System.Net.WebProxy.CreateDefaultProxy();
	}

	internal static DefaultProxySectionInternal GetSection()
	{
		lock (ClassSyncObject)
		{
			return new DefaultProxySectionInternal
			{
				webProxy = GetDefaultProxy_UsingOldMonoCode()
			};
		}
	}
}

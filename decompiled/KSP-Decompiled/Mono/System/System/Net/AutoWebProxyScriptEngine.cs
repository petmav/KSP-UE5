using System.Collections;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Microsoft.Win32;

namespace System.Net;

internal class AutoWebProxyScriptEngine
{
	public Uri AutomaticConfigurationScript { get; set; }

	public bool AutomaticallyDetectSettings { get; set; }

	public AutoWebProxyScriptEngine(WebProxy proxy, bool useRegistry)
	{
	}

	public bool GetProxies(Uri destination, out IList<string> proxyList)
	{
		int syncStatus = 0;
		return GetProxies(destination, out proxyList, ref syncStatus);
	}

	public bool GetProxies(Uri destination, out IList<string> proxyList, ref int syncStatus)
	{
		proxyList = null;
		return false;
	}

	public void Close()
	{
	}

	public void Abort(ref int syncStatus)
	{
	}

	public void CheckForChanges()
	{
	}

	public WebProxyData GetWebProxyData()
	{
		try
		{
			WebProxyData webProxyData;
			if (IsWindows())
			{
				webProxyData = InitializeRegistryGlobalProxy();
				if (webProxyData != null)
				{
					return webProxyData;
				}
			}
			webProxyData = ReadEnvVariables();
			if (webProxyData != null)
			{
				return webProxyData;
			}
		}
		catch (DllNotFoundException)
		{
		}
		return new WebProxyData();
	}

	private WebProxyData ReadEnvVariables()
	{
		string text = Environment.GetEnvironmentVariable("http_proxy") ?? Environment.GetEnvironmentVariable("HTTP_PROXY");
		if (text != null)
		{
			try
			{
				if (!text.StartsWith("http://"))
				{
					text = "http://" + text;
				}
				Uri uri = new Uri(text);
				if (IPAddress.TryParse(uri.Host, out var address))
				{
					if (IPAddress.Any.Equals(address))
					{
						uri = new UriBuilder(uri)
						{
							Host = "127.0.0.1"
						}.Uri;
					}
					else if (IPAddress.IPv6Any.Equals(address))
					{
						uri = new UriBuilder(uri)
						{
							Host = "[::1]"
						}.Uri;
					}
				}
				bool bypassOnLocal = false;
				ArrayList arrayList = new ArrayList();
				string text2 = Environment.GetEnvironmentVariable("no_proxy") ?? Environment.GetEnvironmentVariable("NO_PROXY");
				if (text2 != null)
				{
					string[] array = text2.Split(new char[1] { ',' }, StringSplitOptions.RemoveEmptyEntries);
					foreach (string text3 in array)
					{
						if (text3 != "*.local")
						{
							arrayList.Add(text3);
						}
						else
						{
							bypassOnLocal = true;
						}
					}
				}
				return new WebProxyData
				{
					proxyAddress = uri,
					bypassOnLocal = bypassOnLocal,
					bypassList = CreateBypassList(arrayList)
				};
			}
			catch (UriFormatException)
			{
			}
		}
		return null;
	}

	private static bool IsWindows()
	{
		return Environment.OSVersion.Platform < PlatformID.Unix;
	}

	private WebProxyData InitializeRegistryGlobalProxy()
	{
		if ((int)Registry.GetValue("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "ProxyEnable", 0) > 0)
		{
			string address = "";
			bool bypassOnLocal = false;
			ArrayList arrayList = new ArrayList();
			string text = (string)Registry.GetValue("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "ProxyServer", null);
			string text2 = (string)Registry.GetValue("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "ProxyOverride", null);
			if (text == null)
			{
				return null;
			}
			if (text.Contains("="))
			{
				string[] array = text.Split(new char[1] { ';' }, StringSplitOptions.RemoveEmptyEntries);
				foreach (string text3 in array)
				{
					if (text3.StartsWith("http="))
					{
						address = text3.Substring(5);
						break;
					}
				}
			}
			else
			{
				address = text;
			}
			if (text2 != null)
			{
				string[] array = text2.Split(new char[1] { ';' }, StringSplitOptions.RemoveEmptyEntries);
				foreach (string text4 in array)
				{
					if (text4 != "<local>")
					{
						arrayList.Add(text4);
					}
					else
					{
						bypassOnLocal = true;
					}
				}
			}
			return new WebProxyData
			{
				proxyAddress = ToUri(address),
				bypassOnLocal = bypassOnLocal,
				bypassList = CreateBypassList(arrayList)
			};
		}
		return null;
	}

	private static Uri ToUri(string address)
	{
		if (address == null)
		{
			return null;
		}
		if (address.IndexOf("://", StringComparison.Ordinal) == -1)
		{
			address = "http://" + address;
		}
		return new Uri(address);
	}

	private static ArrayList CreateBypassList(ArrayList al)
	{
		string[] array = al.ToArray(typeof(string)) as string[];
		for (int i = 0; i < array.Length; i++)
		{
			array[i] = "^" + Regex.Escape(array[i]).Replace("\\*", ".*").Replace("\\?", ".") + "$";
		}
		return new ArrayList(array);
	}
}

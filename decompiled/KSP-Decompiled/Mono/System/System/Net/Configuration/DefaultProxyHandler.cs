using System.Collections;
using System.Configuration;
using System.Xml;

namespace System.Net.Configuration;

internal class DefaultProxyHandler : IConfigurationSectionHandler
{
	public virtual object Create(object parent, object configContext, XmlNode section)
	{
		IWebProxy webProxy = parent as IWebProxy;
		if (section.Attributes != null && section.Attributes.Count != 0)
		{
			HandlersUtil.ThrowException("Unrecognized attribute", section);
		}
		foreach (XmlNode childNode in section.ChildNodes)
		{
			XmlNodeType nodeType = childNode.NodeType;
			if (nodeType == XmlNodeType.Whitespace || nodeType == XmlNodeType.Comment)
			{
				continue;
			}
			if (nodeType != XmlNodeType.Element)
			{
				HandlersUtil.ThrowException("Only elements allowed", childNode);
			}
			string text;
			string environmentVariable;
			switch (childNode.Name)
			{
			case "proxy":
			{
				text = HandlersUtil.ExtractAttributeValue("usesystemdefault", childNode, optional: true);
				string text2 = HandlersUtil.ExtractAttributeValue("bypassonlocal", childNode, optional: true);
				environmentVariable = HandlersUtil.ExtractAttributeValue("proxyaddress", childNode, optional: true);
				if (childNode.Attributes != null && childNode.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", childNode);
				}
				webProxy = new WebProxy();
				bool flag = text2 != null && string.Compare(text2, "true", ignoreCase: true) == 0;
				if (!flag && text2 != null && string.Compare(text2, "false", ignoreCase: true) != 0)
				{
					HandlersUtil.ThrowException("Invalid boolean value", childNode);
				}
				if (!(webProxy is WebProxy))
				{
					continue;
				}
				((WebProxy)webProxy).BypassProxyOnLocal = flag;
				if (environmentVariable != null)
				{
					try
					{
						((WebProxy)webProxy).Address = new Uri(environmentVariable);
					}
					catch (UriFormatException)
					{
						goto IL_0143;
					}
					continue;
				}
				goto IL_0143;
			}
			case "bypasslist":
				if (webProxy is WebProxy)
				{
					FillByPassList(childNode, (WebProxy)webProxy);
				}
				continue;
			case "module":
				{
					HandlersUtil.ThrowException("WARNING: module not implemented yet", childNode);
					break;
				}
				IL_0143:
				if (text == null || string.Compare(text, "true", ignoreCase: true) != 0)
				{
					continue;
				}
				environmentVariable = Environment.GetEnvironmentVariable("http_proxy");
				if (environmentVariable == null)
				{
					environmentVariable = Environment.GetEnvironmentVariable("HTTP_PROXY");
				}
				if (environmentVariable == null)
				{
					continue;
				}
				try
				{
					Uri uri = new Uri(environmentVariable);
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
					((WebProxy)webProxy).Address = uri;
				}
				catch (UriFormatException)
				{
				}
				continue;
			}
			HandlersUtil.ThrowException("Unexpected element", childNode);
		}
		return webProxy;
	}

	private static void FillByPassList(XmlNode node, WebProxy proxy)
	{
		ArrayList arrayList = new ArrayList(proxy.BypassArrayList);
		if (node.Attributes != null && node.Attributes.Count != 0)
		{
			HandlersUtil.ThrowException("Unrecognized attribute", node);
		}
		foreach (XmlNode childNode in node.ChildNodes)
		{
			XmlNodeType nodeType = childNode.NodeType;
			if (nodeType == XmlNodeType.Whitespace || nodeType == XmlNodeType.Comment)
			{
				continue;
			}
			if (nodeType != XmlNodeType.Element)
			{
				HandlersUtil.ThrowException("Only elements allowed", childNode);
			}
			switch (childNode.Name)
			{
			case "add":
			{
				string text = HandlersUtil.ExtractAttributeValue("address", childNode);
				if (!arrayList.Contains(text))
				{
					arrayList.Add(text);
				}
				break;
			}
			case "remove":
			{
				string obj = HandlersUtil.ExtractAttributeValue("address", childNode);
				arrayList.Remove(obj);
				break;
			}
			case "clear":
				if (node.Attributes != null && node.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", node);
				}
				arrayList.Clear();
				break;
			default:
				HandlersUtil.ThrowException("Unexpected element", childNode);
				break;
			}
		}
		proxy.BypassList = (string[])arrayList.ToArray(typeof(string));
	}
}

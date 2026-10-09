using System.Configuration;
using System.Xml;

namespace System.Net.Configuration;

internal class NetConfigurationHandler : IConfigurationSectionHandler
{
	public virtual object Create(object parent, object configContext, XmlNode section)
	{
		NetConfig netConfig = new NetConfig();
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
			string name = childNode.Name;
			if (name == "ipv6")
			{
				string text = HandlersUtil.ExtractAttributeValue("enabled", childNode, optional: false);
				if (childNode.Attributes != null && childNode.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", childNode);
				}
				if (text == "true")
				{
					netConfig.ipv6Enabled = true;
				}
				else if (text != "false")
				{
					HandlersUtil.ThrowException("Invalid boolean value", childNode);
				}
			}
			else if (name == "httpWebRequest")
			{
				string text2 = HandlersUtil.ExtractAttributeValue("maximumResponseHeadersLength", childNode, optional: true);
				HandlersUtil.ExtractAttributeValue("useUnsafeHeaderParsing", childNode, optional: true);
				if (childNode.Attributes != null && childNode.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", childNode);
				}
				try
				{
					if (text2 != null)
					{
						int num = int.Parse(text2.Trim());
						if (num < -1)
						{
							HandlersUtil.ThrowException("Must be -1 or >= 0", childNode);
						}
						netConfig.MaxResponseHeadersLength = num;
					}
				}
				catch
				{
					HandlersUtil.ThrowException("Invalid int value", childNode);
				}
			}
			else
			{
				HandlersUtil.ThrowException("Unexpected element", childNode);
			}
		}
		return netConfig;
	}
}

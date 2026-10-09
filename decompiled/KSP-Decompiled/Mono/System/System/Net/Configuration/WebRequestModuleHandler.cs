using System.Collections;
using System.Configuration;
using System.Xml;

namespace System.Net.Configuration;

internal class WebRequestModuleHandler : IConfigurationSectionHandler
{
	public virtual object Create(object parent, object configContext, XmlNode section)
	{
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
			switch (childNode.Name)
			{
			case "clear":
				if (childNode.Attributes != null && childNode.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", childNode);
				}
				WebRequest.PrefixList = new ArrayList();
				break;
			case "add":
				if (childNode.Attributes != null && childNode.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", childNode);
				}
				throw new NotImplementedException();
			case "remove":
				if (childNode.Attributes != null && childNode.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", childNode);
				}
				throw new NotImplementedException();
			default:
				HandlersUtil.ThrowException("Unexpected element", childNode);
				break;
			}
		}
		return null;
	}
}

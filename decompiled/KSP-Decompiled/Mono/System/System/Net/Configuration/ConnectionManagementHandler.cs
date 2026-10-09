using System.Configuration;
using System.Xml;

namespace System.Net.Configuration;

internal class ConnectionManagementHandler : IConfigurationSectionHandler
{
	public virtual object Create(object parent, object configContext, XmlNode section)
	{
		ConnectionManagementData connectionManagementData = new ConnectionManagementData(parent);
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
			if (name == "clear")
			{
				if (childNode.Attributes != null && childNode.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", childNode);
				}
				connectionManagementData.Clear();
				continue;
			}
			string address = HandlersUtil.ExtractAttributeValue("address", childNode);
			if (name == "add")
			{
				string nconns = HandlersUtil.ExtractAttributeValue("maxconnection", childNode, optional: true);
				if (childNode.Attributes != null && childNode.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", childNode);
				}
				connectionManagementData.Add(address, nconns);
			}
			else if (name == "remove")
			{
				if (childNode.Attributes != null && childNode.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", childNode);
				}
				connectionManagementData.Remove(address);
			}
			else
			{
				HandlersUtil.ThrowException("Unexpected element", childNode);
			}
		}
		return connectionManagementData;
	}
}

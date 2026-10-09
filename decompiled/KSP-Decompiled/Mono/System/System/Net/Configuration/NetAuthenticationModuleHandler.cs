using System.Configuration;
using System.Xml;

namespace System.Net.Configuration;

internal class NetAuthenticationModuleHandler : IConfigurationSectionHandler
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
			string name = childNode.Name;
			if (name == "clear")
			{
				if (childNode.Attributes != null && childNode.Attributes.Count != 0)
				{
					HandlersUtil.ThrowException("Unrecognized attribute", childNode);
				}
				AuthenticationManager.Clear();
				continue;
			}
			string typeName = HandlersUtil.ExtractAttributeValue("type", childNode);
			if (childNode.Attributes != null && childNode.Attributes.Count != 0)
			{
				HandlersUtil.ThrowException("Unrecognized attribute", childNode);
			}
			if (name == "add")
			{
				AuthenticationManager.Register(CreateInstance(typeName, childNode));
			}
			else if (name == "remove")
			{
				AuthenticationManager.Unregister(CreateInstance(typeName, childNode));
			}
			else
			{
				HandlersUtil.ThrowException("Unexpected element", childNode);
			}
		}
		return AuthenticationManager.RegisteredModules;
	}

	private static IAuthenticationModule CreateInstance(string typeName, XmlNode node)
	{
		IAuthenticationModule result = null;
		try
		{
			result = (IAuthenticationModule)Activator.CreateInstance(Type.GetType(typeName, throwOnError: true));
		}
		catch (Exception ex)
		{
			HandlersUtil.ThrowException(ex.Message, node);
		}
		return result;
	}
}

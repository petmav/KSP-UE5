using System.Collections;
using System.Xml;

namespace System.Configuration;

public class SingleTagSectionHandler : IConfigurationSectionHandler
{
	public virtual object Create(object parent, object context, XmlNode section)
	{
		Hashtable hashtable = ((parent != null) ? ((Hashtable)parent) : new Hashtable());
		if (section.HasChildNodes)
		{
			throw new ConfigurationException("Child Nodes not allowed.");
		}
		XmlAttributeCollection attributes = section.Attributes;
		for (int i = 0; i < attributes.Count; i++)
		{
			hashtable.Add(attributes[i].Name, attributes[i].Value);
		}
		return hashtable;
	}
}

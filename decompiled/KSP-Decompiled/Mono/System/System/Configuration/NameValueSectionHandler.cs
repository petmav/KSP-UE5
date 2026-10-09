using System.Collections.Specialized;
using System.Xml;

namespace System.Configuration;

public class NameValueSectionHandler : IConfigurationSectionHandler
{
	protected virtual string KeyAttributeName => "key";

	protected virtual string ValueAttributeName => "value";

	public object Create(object parent, object context, XmlNode section)
	{
		return ConfigHelper.GetNameValueCollection(parent as NameValueCollection, section, KeyAttributeName, ValueAttributeName);
	}
}

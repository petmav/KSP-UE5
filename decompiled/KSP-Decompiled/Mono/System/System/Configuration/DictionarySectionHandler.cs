using System.Collections;
using System.Xml;

namespace System.Configuration;

public class DictionarySectionHandler : IConfigurationSectionHandler
{
	protected virtual string KeyAttributeName => "key";

	protected virtual string ValueAttributeName => "value";

	public virtual object Create(object parent, object context, XmlNode section)
	{
		return ConfigHelper.GetDictionary(parent as IDictionary, section, KeyAttributeName, ValueAttributeName);
	}
}

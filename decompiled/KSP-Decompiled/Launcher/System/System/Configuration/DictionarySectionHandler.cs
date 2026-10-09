using System.Xml;
using Unity;

namespace System.Configuration;

public class DictionarySectionHandler : IConfigurationSectionHandler
{
	protected virtual string KeyAttributeName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected virtual string ValueAttributeName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public DictionarySectionHandler()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual object Create(object parent, object context, XmlNode section)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

using System.Xml;
using Unity;

namespace System.Configuration;

public class NameValueSectionHandler : IConfigurationSectionHandler
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

	public NameValueSectionHandler()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Create(object parent, object context, XmlNode section)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

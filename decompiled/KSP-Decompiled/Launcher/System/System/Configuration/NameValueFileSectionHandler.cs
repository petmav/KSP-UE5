using System.Xml;
using Unity;

namespace System.Configuration;

public class NameValueFileSectionHandler : IConfigurationSectionHandler
{
	public NameValueFileSectionHandler()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Create(object parent, object configContext, XmlNode section)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

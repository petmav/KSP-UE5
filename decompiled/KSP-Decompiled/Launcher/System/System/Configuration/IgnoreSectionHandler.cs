using System.Xml;
using Unity;

namespace System.Configuration;

public class IgnoreSectionHandler : IConfigurationSectionHandler
{
	public IgnoreSectionHandler()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual object Create(object parent, object configContext, XmlNode section)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

using System.Xml;
using Unity;

namespace System.Configuration;

public class SingleTagSectionHandler : IConfigurationSectionHandler
{
	public SingleTagSectionHandler()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual object Create(object parent, object context, XmlNode section)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

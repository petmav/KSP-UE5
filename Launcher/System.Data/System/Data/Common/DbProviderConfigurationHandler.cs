using System.Configuration;
using System.Xml;
using Unity;

namespace System.Data.Common;

public class DbProviderConfigurationHandler : IConfigurationSectionHandler
{
	public DbProviderConfigurationHandler()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual object Create(object parent, object configContext, XmlNode section)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

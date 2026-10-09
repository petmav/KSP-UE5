using System.Configuration;
using System.Xml;
using Unity;

namespace System.Data.Common;

public class DbProviderFactoriesConfigurationHandler : IConfigurationSectionHandler
{
	public DbProviderFactoriesConfigurationHandler()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual object Create(object parent, object configContext, XmlNode section)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

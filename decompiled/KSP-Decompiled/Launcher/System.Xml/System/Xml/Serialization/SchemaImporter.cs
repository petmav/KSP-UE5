using System.Security.Permissions;
using System.Xml.Serialization.Advanced;
using Unity;

namespace System.Xml.Serialization;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public abstract class SchemaImporter
{
	public SchemaImporterExtensionCollection Extensions
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal SchemaImporter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

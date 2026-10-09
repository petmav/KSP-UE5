using System.CodeDom;
using System.Security.Permissions;
using Unity;

namespace System.Xml.Serialization;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public abstract class CodeExporter
{
	public CodeAttributeDeclarationCollection IncludeMetadata
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal CodeExporter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

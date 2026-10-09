using System.IO;
using System.Security.Permissions;
using Unity;

namespace System.CodeDom.Compiler;

[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public abstract class CodeParser : ICodeParser
{
	protected CodeParser()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public abstract CodeCompileUnit Parse(TextReader codeStream);
}

using System.Collections.Specialized;
using Unity;

namespace System.CodeDom;

public class CodeCompileUnit
{
	public CodeAttributeDeclarationCollection AssemblyCustomAttributes
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeDirectiveCollection EndDirectives
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeNamespaceCollection Namespaces
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public StringCollection ReferencedAssemblies
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeDirectiveCollection StartDirectives
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}
}

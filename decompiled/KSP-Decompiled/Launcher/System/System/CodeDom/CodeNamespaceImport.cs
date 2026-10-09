using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeNamespaceImport : CodeObject
{
	public CodeLinePragma LinePragma
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string Namespace
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CodeNamespaceImport()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeNamespaceImport(string nameSpace)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

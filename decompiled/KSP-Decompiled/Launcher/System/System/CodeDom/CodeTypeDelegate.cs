using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeTypeDelegate : CodeTypeDeclaration
{
	public CodeParameterDeclarationExpressionCollection Parameters
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeTypeReference ReturnType
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

	public CodeTypeDelegate()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeDelegate(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

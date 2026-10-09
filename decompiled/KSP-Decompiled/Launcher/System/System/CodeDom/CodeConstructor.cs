using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeConstructor : CodeMemberMethod
{
	public CodeExpressionCollection BaseConstructorArgs
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeExpressionCollection ChainedConstructorArgs
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeConstructor()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

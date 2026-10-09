using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeAssignStatement : CodeStatement
{
	public CodeExpression Left
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

	public CodeExpression Right
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

	public CodeAssignStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAssignStatement(CodeExpression left, CodeExpression right)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

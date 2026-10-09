using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeDelegateInvokeExpression : CodeExpression
{
	public CodeExpressionCollection Parameters
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeExpression TargetObject
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

	public CodeDelegateInvokeExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeDelegateInvokeExpression(CodeExpression targetObject)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeDelegateInvokeExpression(CodeExpression targetObject, CodeExpression[] parameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

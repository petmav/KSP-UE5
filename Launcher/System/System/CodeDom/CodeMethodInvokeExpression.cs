using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeMethodInvokeExpression : CodeExpression
{
	public CodeMethodReferenceExpression Method
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

	public CodeExpressionCollection Parameters
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeMethodInvokeExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeMethodInvokeExpression(CodeExpression targetObject, string methodName, CodeExpression[] parameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeMethodInvokeExpression(CodeMethodReferenceExpression method, CodeExpression[] parameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

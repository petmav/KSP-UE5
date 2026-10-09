using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeArrayIndexerExpression : CodeExpression
{
	public CodeExpressionCollection Indices
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

	public CodeArrayIndexerExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArrayIndexerExpression(CodeExpression targetObject, CodeExpression[] indices)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

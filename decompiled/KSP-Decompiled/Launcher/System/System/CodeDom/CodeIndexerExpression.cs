using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeIndexerExpression : CodeExpression
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

	public CodeIndexerExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeIndexerExpression(CodeExpression targetObject, CodeExpression[] indices)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

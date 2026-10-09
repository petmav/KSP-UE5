using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeBinaryOperatorExpression : CodeExpression
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

	public CodeBinaryOperatorType Operator
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(CodeBinaryOperatorType);
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

	public CodeBinaryOperatorExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeBinaryOperatorExpression(CodeExpression left, CodeBinaryOperatorType op, CodeExpression right)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

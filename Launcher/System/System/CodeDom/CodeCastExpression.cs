using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeCastExpression : CodeExpression
{
	public CodeExpression Expression
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

	public CodeTypeReference TargetType
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

	public CodeCastExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCastExpression(CodeTypeReference targetType, CodeExpression expression)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCastExpression(string targetType, CodeExpression expression)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCastExpression(Type targetType, CodeExpression expression)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

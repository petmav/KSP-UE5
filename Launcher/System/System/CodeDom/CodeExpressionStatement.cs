using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeExpressionStatement : CodeStatement
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

	public CodeExpressionStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeExpressionStatement(CodeExpression expression)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

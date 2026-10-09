using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeMethodReturnStatement : CodeStatement
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

	public CodeMethodReturnStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeMethodReturnStatement(CodeExpression expression)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeThrowExceptionStatement : CodeStatement
{
	public CodeExpression ToThrow
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

	public CodeThrowExceptionStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeThrowExceptionStatement(CodeExpression toThrow)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

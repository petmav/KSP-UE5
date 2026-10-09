using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeObjectCreateExpression : CodeExpression
{
	public CodeTypeReference CreateType
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

	public CodeObjectCreateExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeObjectCreateExpression(CodeTypeReference createType, CodeExpression[] parameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeObjectCreateExpression(string createType, CodeExpression[] parameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeObjectCreateExpression(Type createType, CodeExpression[] parameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

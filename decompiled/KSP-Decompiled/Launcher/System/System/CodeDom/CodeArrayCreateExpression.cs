using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeArrayCreateExpression : CodeExpression
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

	public CodeExpressionCollection Initializers
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public int Size
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CodeExpression SizeExpression
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

	public CodeArrayCreateExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArrayCreateExpression(CodeTypeReference createType, CodeExpression size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArrayCreateExpression(CodeTypeReference createType, CodeExpression[] initializers)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArrayCreateExpression(CodeTypeReference createType, int size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArrayCreateExpression(string createType, CodeExpression size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArrayCreateExpression(string createType, CodeExpression[] initializers)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArrayCreateExpression(string createType, int size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArrayCreateExpression(Type createType, CodeExpression size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArrayCreateExpression(Type createType, CodeExpression[] initializers)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArrayCreateExpression(Type createType, int size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeVariableDeclarationStatement : CodeStatement
{
	public CodeExpression InitExpression
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

	public string Name
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

	public CodeTypeReference Type
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

	public CodeVariableDeclarationStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeVariableDeclarationStatement(CodeTypeReference type, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeVariableDeclarationStatement(CodeTypeReference type, string name, CodeExpression initExpression)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeVariableDeclarationStatement(string type, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeVariableDeclarationStatement(string type, string name, CodeExpression initExpression)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeVariableDeclarationStatement(Type type, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeVariableDeclarationStatement(Type type, string name, CodeExpression initExpression)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

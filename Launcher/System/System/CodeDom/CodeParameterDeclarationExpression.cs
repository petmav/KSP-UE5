using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeParameterDeclarationExpression : CodeExpression
{
	public CodeAttributeDeclarationCollection CustomAttributes
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

	public FieldDirection Direction
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(FieldDirection);
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

	public CodeParameterDeclarationExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeParameterDeclarationExpression(CodeTypeReference type, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeParameterDeclarationExpression(string type, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeParameterDeclarationExpression(Type type, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

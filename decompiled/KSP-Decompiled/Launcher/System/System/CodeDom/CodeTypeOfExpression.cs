using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeTypeOfExpression : CodeExpression
{
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

	public CodeTypeOfExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeOfExpression(CodeTypeReference type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeOfExpression(string type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeOfExpression(Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

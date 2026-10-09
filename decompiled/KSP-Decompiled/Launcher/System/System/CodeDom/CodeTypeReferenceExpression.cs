using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeTypeReferenceExpression : CodeExpression
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

	public CodeTypeReferenceExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReferenceExpression(CodeTypeReference type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReferenceExpression(string type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReferenceExpression(Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

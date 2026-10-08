using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeMethodReferenceExpression : CodeExpression
{
	public string MethodName
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

	public CodeExpression TargetObject
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

	public CodeTypeReferenceCollection TypeArguments
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeMethodReferenceExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeMethodReferenceExpression(CodeExpression targetObject, string methodName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeMethodReferenceExpression(CodeExpression targetObject, string methodName, CodeTypeReference[] typeParameters)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

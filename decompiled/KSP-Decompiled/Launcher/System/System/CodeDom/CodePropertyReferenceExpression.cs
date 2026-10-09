using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodePropertyReferenceExpression : CodeExpression
{
	public string PropertyName
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

	public CodePropertyReferenceExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodePropertyReferenceExpression(CodeExpression targetObject, string propertyName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

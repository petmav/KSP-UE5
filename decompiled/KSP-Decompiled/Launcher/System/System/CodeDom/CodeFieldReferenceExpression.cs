using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeFieldReferenceExpression : CodeExpression
{
	public string FieldName
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

	public CodeFieldReferenceExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeFieldReferenceExpression(CodeExpression targetObject, string fieldName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

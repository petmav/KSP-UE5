using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodePrimitiveExpression : CodeExpression
{
	public object Value
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

	public CodePrimitiveExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodePrimitiveExpression(object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

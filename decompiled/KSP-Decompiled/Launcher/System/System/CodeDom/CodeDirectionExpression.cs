using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeDirectionExpression : CodeExpression
{
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

	public CodeExpression Expression
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

	public CodeDirectionExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeDirectionExpression(FieldDirection direction, CodeExpression expression)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

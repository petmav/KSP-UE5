using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeDefaultValueExpression : CodeExpression
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

	public CodeDefaultValueExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeDefaultValueExpression(CodeTypeReference type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

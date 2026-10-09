using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeThisReferenceExpression : CodeExpression
{
	public CodeThisReferenceExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

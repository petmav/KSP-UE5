using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeBaseReferenceExpression : CodeExpression
{
	public CodeBaseReferenceExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

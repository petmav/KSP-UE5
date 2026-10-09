using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodePropertySetValueReferenceExpression : CodeExpression
{
	public CodePropertySetValueReferenceExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

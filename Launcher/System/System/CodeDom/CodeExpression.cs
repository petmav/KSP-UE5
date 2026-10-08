using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeExpression : CodeObject
{
	public CodeExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

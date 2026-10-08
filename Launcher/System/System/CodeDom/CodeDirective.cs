using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeDirective : CodeObject
{
	public CodeDirective()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

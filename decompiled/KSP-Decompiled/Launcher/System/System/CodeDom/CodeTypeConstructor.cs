using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeTypeConstructor : CodeMemberMethod
{
	public CodeTypeConstructor()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeEntryPointMethod : CodeMemberMethod
{
	public CodeEntryPointMethod()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeGotoStatement : CodeStatement
{
	public string Label
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

	public CodeGotoStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeGotoStatement(string label)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

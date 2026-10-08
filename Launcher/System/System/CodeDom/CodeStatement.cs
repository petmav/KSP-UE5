using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeStatement : CodeObject
{
	public CodeDirectiveCollection EndDirectives
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeLinePragma LinePragma
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

	public CodeDirectiveCollection StartDirectives
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

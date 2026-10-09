using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeSnippetTypeMember : CodeTypeMember
{
	public string Text
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

	public CodeSnippetTypeMember()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeSnippetTypeMember(string text)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

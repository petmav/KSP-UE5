using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeSnippetStatement : CodeStatement
{
	public string Value
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

	public CodeSnippetStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeSnippetStatement(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

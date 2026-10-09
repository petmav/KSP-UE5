using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeSnippetExpression : CodeExpression
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

	public CodeSnippetExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeSnippetExpression(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

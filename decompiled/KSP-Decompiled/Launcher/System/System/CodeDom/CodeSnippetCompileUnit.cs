using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeSnippetCompileUnit : CodeCompileUnit
{
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

	public CodeSnippetCompileUnit()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeSnippetCompileUnit(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

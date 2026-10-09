using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeCommentStatement : CodeStatement
{
	public CodeComment Comment
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

	public CodeCommentStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCommentStatement(CodeComment comment)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCommentStatement(string text)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCommentStatement(string text, bool docComment)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

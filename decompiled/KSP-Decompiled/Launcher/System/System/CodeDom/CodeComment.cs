using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeComment : CodeObject
{
	public bool DocComment
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

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

	public CodeComment()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeComment(string text)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeComment(string text, bool docComment)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

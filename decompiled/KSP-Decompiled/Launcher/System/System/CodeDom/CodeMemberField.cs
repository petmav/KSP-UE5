using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeMemberField : CodeTypeMember
{
	public CodeExpression InitExpression
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

	public CodeTypeReference Type
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

	public CodeMemberField()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeMemberField(CodeTypeReference type, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeMemberField(string type, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeMemberField(Type type, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

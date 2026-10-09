using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeAttachEventStatement : CodeStatement
{
	public CodeEventReferenceExpression Event
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

	public CodeExpression Listener
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

	public CodeAttachEventStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAttachEventStatement(CodeEventReferenceExpression eventRef, CodeExpression listener)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAttachEventStatement(CodeExpression targetObject, string eventName, CodeExpression listener)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

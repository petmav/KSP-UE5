using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeRemoveEventStatement : CodeStatement
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

	public CodeRemoveEventStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeRemoveEventStatement(CodeEventReferenceExpression eventRef, CodeExpression listener)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeRemoveEventStatement(CodeExpression targetObject, string eventName, CodeExpression listener)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

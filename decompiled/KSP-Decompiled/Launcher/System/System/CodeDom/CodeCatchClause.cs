using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeCatchClause
{
	public CodeTypeReference CatchExceptionType
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

	public string LocalName
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

	public CodeStatementCollection Statements
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeCatchClause()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCatchClause(string localName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCatchClause(string localName, CodeTypeReference catchExceptionType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeCatchClause(string localName, CodeTypeReference catchExceptionType, CodeStatement[] statements)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeMemberProperty : CodeTypeMember
{
	public CodeStatementCollection GetStatements
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool HasGet
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

	public bool HasSet
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

	public CodeTypeReferenceCollection ImplementationTypes
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeParameterDeclarationExpressionCollection Parameters
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeTypeReference PrivateImplementationType
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

	public CodeStatementCollection SetStatements
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
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

	public CodeMemberProperty()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

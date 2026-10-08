using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeTypeReference : CodeObject
{
	public CodeTypeReference ArrayElementType
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

	public int ArrayRank
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string BaseType
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

	public CodeTypeReferenceOptions Options
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(CodeTypeReferenceOptions);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CodeTypeReferenceCollection TypeArguments
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeTypeReference()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReference(CodeTypeParameter typeParameter)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReference(CodeTypeReference arrayType, int rank)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReference(string typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReference(string typeName, CodeTypeReferenceOptions codeTypeReferenceOption)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReference(string typeName, CodeTypeReference[] typeArguments)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReference(string baseType, int rank)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReference(Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeTypeReference(Type type, CodeTypeReferenceOptions codeTypeReferenceOption)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

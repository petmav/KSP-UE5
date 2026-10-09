using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeAttributeDeclaration
{
	public CodeAttributeArgumentCollection Arguments
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeTypeReference AttributeType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Name
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

	public CodeAttributeDeclaration()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAttributeDeclaration(CodeTypeReference attributeType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAttributeDeclaration(CodeTypeReference attributeType, CodeAttributeArgument[] arguments)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAttributeDeclaration(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAttributeDeclaration(string name, CodeAttributeArgument[] arguments)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

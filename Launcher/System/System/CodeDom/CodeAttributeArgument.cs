using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeAttributeArgument
{
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

	public CodeExpression Value
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

	public CodeAttributeArgument()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAttributeArgument(CodeExpression value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeAttributeArgument(string name, CodeExpression value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

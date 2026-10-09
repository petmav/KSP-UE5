using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
public class CodeVariableReferenceExpression : CodeExpression
{
	public string VariableName
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

	public CodeVariableReferenceExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeVariableReferenceExpression(string variableName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

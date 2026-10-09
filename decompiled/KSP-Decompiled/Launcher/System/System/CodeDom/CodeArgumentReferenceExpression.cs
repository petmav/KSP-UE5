using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeArgumentReferenceExpression : CodeExpression
{
	public string ParameterName
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

	public CodeArgumentReferenceExpression()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeArgumentReferenceExpression(string parameterName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

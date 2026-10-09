using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeLabeledStatement : CodeStatement
{
	public string Label
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

	public CodeStatement Statement
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

	public CodeLabeledStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeLabeledStatement(string label)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeLabeledStatement(string label, CodeStatement statement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

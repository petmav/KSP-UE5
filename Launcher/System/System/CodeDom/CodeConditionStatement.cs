using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeConditionStatement : CodeStatement
{
	public CodeExpression Condition
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

	public CodeStatementCollection FalseStatements
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeStatementCollection TrueStatements
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public CodeConditionStatement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeConditionStatement(CodeExpression condition, CodeStatement[] trueStatements)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeConditionStatement(CodeExpression condition, CodeStatement[] trueStatements, CodeStatement[] falseStatements)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

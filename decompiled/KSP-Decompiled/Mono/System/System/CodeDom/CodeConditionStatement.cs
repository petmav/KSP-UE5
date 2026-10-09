namespace System.CodeDom;

[Serializable]
public class CodeConditionStatement : CodeStatement
{
	public CodeExpression Condition { get; set; }

	public CodeStatementCollection TrueStatements { get; } = new CodeStatementCollection();


	public CodeStatementCollection FalseStatements { get; } = new CodeStatementCollection();


	public CodeConditionStatement()
	{
	}

	public CodeConditionStatement(CodeExpression condition, params CodeStatement[] trueStatements)
	{
		Condition = condition;
		TrueStatements.AddRange(trueStatements);
	}

	public CodeConditionStatement(CodeExpression condition, CodeStatement[] trueStatements, CodeStatement[] falseStatements)
	{
		Condition = condition;
		TrueStatements.AddRange(trueStatements);
		FalseStatements.AddRange(falseStatements);
	}
}

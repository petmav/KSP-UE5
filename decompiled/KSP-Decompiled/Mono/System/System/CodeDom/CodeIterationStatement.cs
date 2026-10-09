namespace System.CodeDom;

[Serializable]
public class CodeIterationStatement : CodeStatement
{
	public CodeStatement InitStatement { get; set; }

	public CodeExpression TestExpression { get; set; }

	public CodeStatement IncrementStatement { get; set; }

	public CodeStatementCollection Statements { get; } = new CodeStatementCollection();


	public CodeIterationStatement()
	{
	}

	public CodeIterationStatement(CodeStatement initStatement, CodeExpression testExpression, CodeStatement incrementStatement, params CodeStatement[] statements)
	{
		InitStatement = initStatement;
		TestExpression = testExpression;
		IncrementStatement = incrementStatement;
		Statements.AddRange(statements);
	}
}

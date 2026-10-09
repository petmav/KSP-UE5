namespace System.CodeDom;

[Serializable]
public class CodeExpressionStatement : CodeStatement
{
	public CodeExpression Expression { get; set; }

	public CodeExpressionStatement()
	{
	}

	public CodeExpressionStatement(CodeExpression expression)
	{
		Expression = expression;
	}
}

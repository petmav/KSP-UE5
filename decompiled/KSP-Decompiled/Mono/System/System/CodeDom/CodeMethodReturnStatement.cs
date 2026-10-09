namespace System.CodeDom;

[Serializable]
public class CodeMethodReturnStatement : CodeStatement
{
	public CodeExpression Expression { get; set; }

	public CodeMethodReturnStatement()
	{
	}

	public CodeMethodReturnStatement(CodeExpression expression)
	{
		Expression = expression;
	}
}

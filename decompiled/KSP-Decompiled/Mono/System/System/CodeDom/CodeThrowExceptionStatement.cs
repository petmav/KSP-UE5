namespace System.CodeDom;

[Serializable]
public class CodeThrowExceptionStatement : CodeStatement
{
	public CodeExpression ToThrow { get; set; }

	public CodeThrowExceptionStatement()
	{
	}

	public CodeThrowExceptionStatement(CodeExpression toThrow)
	{
		ToThrow = toThrow;
	}
}

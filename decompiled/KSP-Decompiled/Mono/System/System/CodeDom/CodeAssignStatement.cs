namespace System.CodeDom;

[Serializable]
public class CodeAssignStatement : CodeStatement
{
	public CodeExpression Left { get; set; }

	public CodeExpression Right { get; set; }

	public CodeAssignStatement()
	{
	}

	public CodeAssignStatement(CodeExpression left, CodeExpression right)
	{
		Left = left;
		Right = right;
	}
}

namespace System.CodeDom;

[Serializable]
public class CodeBinaryOperatorExpression : CodeExpression
{
	public CodeExpression Right { get; set; }

	public CodeExpression Left { get; set; }

	public CodeBinaryOperatorType Operator { get; set; }

	public CodeBinaryOperatorExpression()
	{
	}

	public CodeBinaryOperatorExpression(CodeExpression left, CodeBinaryOperatorType op, CodeExpression right)
	{
		Right = right;
		Operator = op;
		Left = left;
	}
}

namespace System.CodeDom;

[Serializable]
public class CodeDirectionExpression : CodeExpression
{
	public CodeExpression Expression { get; set; }

	public FieldDirection Direction { get; set; }

	public CodeDirectionExpression()
	{
	}

	public CodeDirectionExpression(FieldDirection direction, CodeExpression expression)
	{
		Expression = expression;
		Direction = direction;
	}
}

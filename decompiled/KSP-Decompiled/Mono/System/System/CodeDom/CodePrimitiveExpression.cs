namespace System.CodeDom;

[Serializable]
public class CodePrimitiveExpression : CodeExpression
{
	public object Value { get; set; }

	public CodePrimitiveExpression()
	{
	}

	public CodePrimitiveExpression(object value)
	{
		Value = value;
	}
}

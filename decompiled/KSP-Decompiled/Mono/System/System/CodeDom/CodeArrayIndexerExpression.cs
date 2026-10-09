namespace System.CodeDom;

[Serializable]
public class CodeArrayIndexerExpression : CodeExpression
{
	private CodeExpressionCollection _indices;

	public CodeExpression TargetObject { get; set; }

	public CodeExpressionCollection Indices => _indices ?? (_indices = new CodeExpressionCollection());

	public CodeArrayIndexerExpression()
	{
	}

	public CodeArrayIndexerExpression(CodeExpression targetObject, params CodeExpression[] indices)
	{
		TargetObject = targetObject;
		Indices.AddRange(indices);
	}
}

namespace System.CodeDom;

[Serializable]
public class CodeIndexerExpression : CodeExpression
{
	private CodeExpressionCollection _indices;

	public CodeExpression TargetObject { get; set; }

	public CodeExpressionCollection Indices => _indices ?? (_indices = new CodeExpressionCollection());

	public CodeIndexerExpression()
	{
	}

	public CodeIndexerExpression(CodeExpression targetObject, params CodeExpression[] indices)
	{
		TargetObject = targetObject;
		Indices.AddRange(indices);
	}
}

namespace System.CodeDom;

[Serializable]
public class CodeDelegateInvokeExpression : CodeExpression
{
	public CodeExpression TargetObject { get; set; }

	public CodeExpressionCollection Parameters { get; } = new CodeExpressionCollection();


	public CodeDelegateInvokeExpression()
	{
	}

	public CodeDelegateInvokeExpression(CodeExpression targetObject)
	{
		TargetObject = targetObject;
	}

	public CodeDelegateInvokeExpression(CodeExpression targetObject, params CodeExpression[] parameters)
	{
		TargetObject = targetObject;
		Parameters.AddRange(parameters);
	}
}

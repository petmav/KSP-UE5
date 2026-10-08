namespace System.CodeDom;

[Serializable]
public class CodeMethodInvokeExpression : CodeExpression
{
	private CodeMethodReferenceExpression _method;

	public CodeMethodReferenceExpression Method
	{
		get
		{
			return _method ?? (_method = new CodeMethodReferenceExpression());
		}
		set
		{
			_method = value;
		}
	}

	public CodeExpressionCollection Parameters { get; } = new CodeExpressionCollection();


	public CodeMethodInvokeExpression()
	{
	}

	public CodeMethodInvokeExpression(CodeMethodReferenceExpression method, params CodeExpression[] parameters)
	{
		_method = method;
		Parameters.AddRange(parameters);
	}

	public CodeMethodInvokeExpression(CodeExpression targetObject, string methodName, params CodeExpression[] parameters)
	{
		_method = new CodeMethodReferenceExpression(targetObject, methodName);
		Parameters.AddRange(parameters);
	}
}

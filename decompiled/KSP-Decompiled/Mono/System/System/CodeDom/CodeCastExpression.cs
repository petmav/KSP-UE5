namespace System.CodeDom;

[Serializable]
public class CodeCastExpression : CodeExpression
{
	private CodeTypeReference _targetType;

	public CodeTypeReference TargetType
	{
		get
		{
			return _targetType ?? (_targetType = new CodeTypeReference(""));
		}
		set
		{
			_targetType = value;
		}
	}

	public CodeExpression Expression { get; set; }

	public CodeCastExpression()
	{
	}

	public CodeCastExpression(CodeTypeReference targetType, CodeExpression expression)
	{
		TargetType = targetType;
		Expression = expression;
	}

	public CodeCastExpression(string targetType, CodeExpression expression)
	{
		TargetType = new CodeTypeReference(targetType);
		Expression = expression;
	}

	public CodeCastExpression(Type targetType, CodeExpression expression)
	{
		TargetType = new CodeTypeReference(targetType);
		Expression = expression;
	}
}

namespace System.CodeDom;

[Serializable]
public class CodeDelegateCreateExpression : CodeExpression
{
	private CodeTypeReference _delegateType;

	private string _methodName;

	public CodeTypeReference DelegateType
	{
		get
		{
			return _delegateType ?? (_delegateType = new CodeTypeReference(""));
		}
		set
		{
			_delegateType = value;
		}
	}

	public CodeExpression TargetObject { get; set; }

	public string MethodName
	{
		get
		{
			return _methodName ?? string.Empty;
		}
		set
		{
			_methodName = value;
		}
	}

	public CodeDelegateCreateExpression()
	{
	}

	public CodeDelegateCreateExpression(CodeTypeReference delegateType, CodeExpression targetObject, string methodName)
	{
		_delegateType = delegateType;
		TargetObject = targetObject;
		_methodName = methodName;
	}
}

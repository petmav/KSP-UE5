namespace System.CodeDom;

[Serializable]
public class CodePropertyReferenceExpression : CodeExpression
{
	private string _propertyName;

	public CodeExpression TargetObject { get; set; }

	public string PropertyName
	{
		get
		{
			return _propertyName ?? string.Empty;
		}
		set
		{
			_propertyName = value;
		}
	}

	public CodePropertyReferenceExpression()
	{
	}

	public CodePropertyReferenceExpression(CodeExpression targetObject, string propertyName)
	{
		TargetObject = targetObject;
		PropertyName = propertyName;
	}
}

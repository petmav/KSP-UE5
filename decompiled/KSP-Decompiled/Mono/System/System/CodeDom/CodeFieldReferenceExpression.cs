namespace System.CodeDom;

[Serializable]
public class CodeFieldReferenceExpression : CodeExpression
{
	private string _fieldName;

	public CodeExpression TargetObject { get; set; }

	public string FieldName
	{
		get
		{
			return _fieldName ?? string.Empty;
		}
		set
		{
			_fieldName = value;
		}
	}

	public CodeFieldReferenceExpression()
	{
	}

	public CodeFieldReferenceExpression(CodeExpression targetObject, string fieldName)
	{
		TargetObject = targetObject;
		FieldName = fieldName;
	}
}

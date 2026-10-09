namespace System.CodeDom;

[Serializable]
public class CodeVariableReferenceExpression : CodeExpression
{
	private string _variableName;

	public string VariableName
	{
		get
		{
			return _variableName ?? string.Empty;
		}
		set
		{
			_variableName = value;
		}
	}

	public CodeVariableReferenceExpression()
	{
	}

	public CodeVariableReferenceExpression(string variableName)
	{
		_variableName = variableName;
	}
}

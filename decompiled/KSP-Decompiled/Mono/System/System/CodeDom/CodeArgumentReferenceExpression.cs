namespace System.CodeDom;

[Serializable]
public class CodeArgumentReferenceExpression : CodeExpression
{
	private string _parameterName;

	public string ParameterName
	{
		get
		{
			return _parameterName ?? string.Empty;
		}
		set
		{
			_parameterName = value;
		}
	}

	public CodeArgumentReferenceExpression()
	{
	}

	public CodeArgumentReferenceExpression(string parameterName)
	{
		_parameterName = parameterName;
	}
}

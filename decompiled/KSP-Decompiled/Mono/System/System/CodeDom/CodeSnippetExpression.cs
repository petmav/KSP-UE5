namespace System.CodeDom;

[Serializable]
public class CodeSnippetExpression : CodeExpression
{
	private string _value;

	public string Value
	{
		get
		{
			return _value ?? string.Empty;
		}
		set
		{
			_value = value;
		}
	}

	public CodeSnippetExpression()
	{
	}

	public CodeSnippetExpression(string value)
	{
		Value = value;
	}
}

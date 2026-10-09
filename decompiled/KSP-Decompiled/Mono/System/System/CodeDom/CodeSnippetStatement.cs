namespace System.CodeDom;

[Serializable]
public class CodeSnippetStatement : CodeStatement
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

	public CodeSnippetStatement()
	{
	}

	public CodeSnippetStatement(string value)
	{
		Value = value;
	}
}

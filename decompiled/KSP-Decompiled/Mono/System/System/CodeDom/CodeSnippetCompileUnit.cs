namespace System.CodeDom;

[Serializable]
public class CodeSnippetCompileUnit : CodeCompileUnit
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

	public CodeLinePragma LinePragma { get; set; }

	public CodeSnippetCompileUnit()
	{
	}

	public CodeSnippetCompileUnit(string value)
	{
		Value = value;
	}
}

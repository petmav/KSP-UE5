namespace System.CodeDom;

[Serializable]
public class CodeSnippetTypeMember : CodeTypeMember
{
	private string _text;

	public string Text
	{
		get
		{
			return _text ?? string.Empty;
		}
		set
		{
			_text = value;
		}
	}

	public CodeSnippetTypeMember()
	{
	}

	public CodeSnippetTypeMember(string text)
	{
		Text = text;
	}
}

namespace System.CodeDom;

[Serializable]
public class CodeComment : CodeObject
{
	private string _text;

	public bool DocComment { get; set; }

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

	public CodeComment()
	{
	}

	public CodeComment(string text)
	{
		Text = text;
	}

	public CodeComment(string text, bool docComment)
	{
		Text = text;
		DocComment = docComment;
	}
}

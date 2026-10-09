namespace System.CodeDom;

[Serializable]
public class CodeCommentStatement : CodeStatement
{
	public CodeComment Comment { get; set; }

	public CodeCommentStatement()
	{
	}

	public CodeCommentStatement(CodeComment comment)
	{
		Comment = comment;
	}

	public CodeCommentStatement(string text)
	{
		Comment = new CodeComment(text);
	}

	public CodeCommentStatement(string text, bool docComment)
	{
		Comment = new CodeComment(text, docComment);
	}
}

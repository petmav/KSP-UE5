namespace System.CodeDom;

[Serializable]
public class CodeStatement : CodeObject
{
	private CodeDirectiveCollection _startDirectives;

	private CodeDirectiveCollection _endDirectives;

	public CodeLinePragma LinePragma { get; set; }

	public CodeDirectiveCollection StartDirectives => _startDirectives ?? (_startDirectives = new CodeDirectiveCollection());

	public CodeDirectiveCollection EndDirectives => _endDirectives ?? (_endDirectives = new CodeDirectiveCollection());
}

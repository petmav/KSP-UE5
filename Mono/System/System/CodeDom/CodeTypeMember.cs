namespace System.CodeDom;

[Serializable]
public class CodeTypeMember : CodeObject
{
	private string _name;

	private CodeAttributeDeclarationCollection _customAttributes;

	private CodeDirectiveCollection _startDirectives;

	private CodeDirectiveCollection _endDirectives;

	public string Name
	{
		get
		{
			return _name ?? string.Empty;
		}
		set
		{
			_name = value;
		}
	}

	public MemberAttributes Attributes { get; set; } = (MemberAttributes)20482;


	public CodeAttributeDeclarationCollection CustomAttributes
	{
		get
		{
			return _customAttributes ?? (_customAttributes = new CodeAttributeDeclarationCollection());
		}
		set
		{
			_customAttributes = value;
		}
	}

	public CodeLinePragma LinePragma { get; set; }

	public CodeCommentStatementCollection Comments { get; } = new CodeCommentStatementCollection();


	public CodeDirectiveCollection StartDirectives => _startDirectives ?? (_startDirectives = new CodeDirectiveCollection());

	public CodeDirectiveCollection EndDirectives => _endDirectives ?? (_endDirectives = new CodeDirectiveCollection());
}

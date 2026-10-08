namespace System.CodeDom;

[Serializable]
public class CodeMemberProperty : CodeTypeMember
{
	private CodeTypeReference _type;

	private bool _hasGet;

	private bool _hasSet;

	private CodeTypeReferenceCollection _implementationTypes;

	public CodeTypeReference PrivateImplementationType { get; set; }

	public CodeTypeReferenceCollection ImplementationTypes => _implementationTypes ?? (_implementationTypes = new CodeTypeReferenceCollection());

	public CodeTypeReference Type
	{
		get
		{
			return _type ?? (_type = new CodeTypeReference(""));
		}
		set
		{
			_type = value;
		}
	}

	public bool HasGet
	{
		get
		{
			if (!_hasGet)
			{
				return GetStatements.Count > 0;
			}
			return true;
		}
		set
		{
			_hasGet = value;
			if (!value)
			{
				GetStatements.Clear();
			}
		}
	}

	public bool HasSet
	{
		get
		{
			if (!_hasSet)
			{
				return SetStatements.Count > 0;
			}
			return true;
		}
		set
		{
			_hasSet = value;
			if (!value)
			{
				SetStatements.Clear();
			}
		}
	}

	public CodeStatementCollection GetStatements { get; } = new CodeStatementCollection();


	public CodeStatementCollection SetStatements { get; } = new CodeStatementCollection();


	public CodeParameterDeclarationExpressionCollection Parameters { get; } = new CodeParameterDeclarationExpressionCollection();

}

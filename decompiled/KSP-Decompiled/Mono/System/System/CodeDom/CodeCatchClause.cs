namespace System.CodeDom;

[Serializable]
public class CodeCatchClause
{
	private CodeStatementCollection _statements;

	private CodeTypeReference _catchExceptionType;

	private string _localName;

	public string LocalName
	{
		get
		{
			return _localName ?? string.Empty;
		}
		set
		{
			_localName = value;
		}
	}

	public CodeTypeReference CatchExceptionType
	{
		get
		{
			return _catchExceptionType ?? (_catchExceptionType = new CodeTypeReference(typeof(Exception)));
		}
		set
		{
			_catchExceptionType = value;
		}
	}

	public CodeStatementCollection Statements => _statements ?? (_statements = new CodeStatementCollection());

	public CodeCatchClause()
	{
	}

	public CodeCatchClause(string localName)
	{
		_localName = localName;
	}

	public CodeCatchClause(string localName, CodeTypeReference catchExceptionType)
	{
		_localName = localName;
		_catchExceptionType = catchExceptionType;
	}

	public CodeCatchClause(string localName, CodeTypeReference catchExceptionType, params CodeStatement[] statements)
	{
		_localName = localName;
		_catchExceptionType = catchExceptionType;
		Statements.AddRange(statements);
	}
}

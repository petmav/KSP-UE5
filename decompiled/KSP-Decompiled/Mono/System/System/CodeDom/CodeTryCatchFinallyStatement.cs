namespace System.CodeDom;

[Serializable]
public class CodeTryCatchFinallyStatement : CodeStatement
{
	public CodeStatementCollection TryStatements { get; } = new CodeStatementCollection();


	public CodeCatchClauseCollection CatchClauses { get; } = new CodeCatchClauseCollection();


	public CodeStatementCollection FinallyStatements { get; } = new CodeStatementCollection();


	public CodeTryCatchFinallyStatement()
	{
	}

	public CodeTryCatchFinallyStatement(CodeStatement[] tryStatements, CodeCatchClause[] catchClauses)
	{
		TryStatements.AddRange(tryStatements);
		CatchClauses.AddRange(catchClauses);
	}

	public CodeTryCatchFinallyStatement(CodeStatement[] tryStatements, CodeCatchClause[] catchClauses, CodeStatement[] finallyStatements)
	{
		TryStatements.AddRange(tryStatements);
		CatchClauses.AddRange(catchClauses);
		FinallyStatements.AddRange(finallyStatements);
	}
}

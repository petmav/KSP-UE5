using System.Data.Common;
using Unity;

namespace System.Data.OleDb;

public sealed class OleDbCommandBuilder : DbCommandBuilder
{
	public new OleDbDataAdapter DataAdapter
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public OleDbCommandBuilder()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbCommandBuilder(OleDbDataAdapter adapter)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void ApplyParameterInfo(DbParameter parameter, DataRow datarow, StatementType statementType, bool whereClause)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void DeriveParameters(OleDbCommand command)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override string GetParameterName(int parameterOrdinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override string GetParameterName(string parameterName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override string GetParameterPlaceholder(int parameterOrdinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string QuoteIdentifier(string unquotedIdentifier, OleDbConnection connection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override void SetRowUpdatingHandler(DbDataAdapter adapter)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public string UnquoteIdentifier(string quotedIdentifier, OleDbConnection connection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

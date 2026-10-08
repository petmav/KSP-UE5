using System.Data.Common;
using Unity;

namespace System.Data.Odbc;

public sealed class OdbcCommandBuilder : DbCommandBuilder
{
	public new OdbcDataAdapter DataAdapter
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public OdbcCommandBuilder()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcCommandBuilder(OdbcDataAdapter adapter)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void ApplyParameterInfo(DbParameter parameter, DataRow datarow, StatementType statementType, bool whereClause)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void DeriveParameters(OdbcCommand command)
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

	public string QuoteIdentifier(string unquotedIdentifier, OdbcConnection connection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override void SetRowUpdatingHandler(DbDataAdapter adapter)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public string UnquoteIdentifier(string quotedIdentifier, OdbcConnection connection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

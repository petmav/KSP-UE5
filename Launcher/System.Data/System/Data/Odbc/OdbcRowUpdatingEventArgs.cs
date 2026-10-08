using System.Data.Common;
using Unity;

namespace System.Data.Odbc;

public sealed class OdbcRowUpdatingEventArgs : RowUpdatingEventArgs
{
	public new OdbcCommand Command
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public OdbcRowUpdatingEventArgs(DataRow row, IDbCommand command, StatementType statementType, DataTableMapping tableMapping)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

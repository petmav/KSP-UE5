using System.Data.Common;
using Unity;

namespace System.Data.Odbc;

public sealed class OdbcRowUpdatedEventArgs : RowUpdatedEventArgs
{
	public OdbcRowUpdatedEventArgs(DataRow row, IDbCommand command, StatementType statementType, DataTableMapping tableMapping)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

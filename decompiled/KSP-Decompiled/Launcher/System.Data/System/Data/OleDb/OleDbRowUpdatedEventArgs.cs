using System.Data.Common;
using Unity;

namespace System.Data.OleDb;

public sealed class OleDbRowUpdatedEventArgs : RowUpdatedEventArgs
{
	public OleDbRowUpdatedEventArgs(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

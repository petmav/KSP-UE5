using System.Data.Common;
using Unity;

namespace System.Data.OleDb;

public sealed class OleDbRowUpdatingEventArgs : RowUpdatingEventArgs
{
	public new OleDbCommand Command
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public OleDbRowUpdatingEventArgs(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

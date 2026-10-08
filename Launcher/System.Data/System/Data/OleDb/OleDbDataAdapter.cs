using System.ComponentModel;
using System.Data.Common;
using Unity;

namespace System.Data.OleDb;

[DefaultEvent("RowUpdated")]
[ToolboxItem("Microsoft.VSDesigner.Data.VS.OleDbDataAdapterToolboxItem, Microsoft.VSDesigner, Version=10.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
[Designer("Microsoft.VSDesigner.Data.VS.OleDbDataAdapterDesigner, Microsoft.VSDesigner, Version=10.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
public sealed class OleDbDataAdapter : DbDataAdapter
{
	public new OleDbCommand DeleteCommand
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public new OleDbCommand InsertCommand
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public new OleDbCommand SelectCommand
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public new OleDbCommand UpdateCommand
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event OleDbRowUpdatedEventHandler RowUpdated
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event OleDbRowUpdatingEventHandler RowUpdating
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public OleDbDataAdapter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbDataAdapter(OleDbCommand selectCommand)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbDataAdapter(string selectCommandText, OleDbConnection selectConnection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbDataAdapter(string selectCommandText, string selectConnectionString)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Fill(DataSet dataSet, object ADODBRecordSet, string srcTable)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int Fill(DataTable dataTable, object ADODBRecordSet)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}
}

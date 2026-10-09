using System.ComponentModel;
using System.Data.Common;
using Unity;

namespace System.Data.Odbc;

[DefaultEvent("RowUpdated")]
[Designer("Microsoft.VSDesigner.Data.VS.OdbcDataAdapterDesigner, Microsoft.VSDesigner, Version=10.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
[ToolboxItem("Microsoft.VSDesigner.Data.VS.OdbcDataAdapterToolboxItem, Microsoft.VSDesigner, Version=10.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
public sealed class OdbcDataAdapter : DbDataAdapter
{
	public new OdbcCommand DeleteCommand
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public new OdbcCommand InsertCommand
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public new OdbcCommand SelectCommand
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public new OdbcCommand UpdateCommand
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event OdbcRowUpdatedEventHandler RowUpdated
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

	public event OdbcRowUpdatingEventHandler RowUpdating
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

	public OdbcDataAdapter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcDataAdapter(OdbcCommand selectCommand)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcDataAdapter(string selectCommandText, OdbcConnection selectConnection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcDataAdapter(string selectCommandText, string selectConnectionString)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

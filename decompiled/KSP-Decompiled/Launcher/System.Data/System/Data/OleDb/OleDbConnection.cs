using System.ComponentModel;
using System.Data.Common;
using System.EnterpriseServices;
using Unity;

namespace System.Data.OleDb;

[DefaultEvent("InfoMessage")]
public sealed class OleDbConnection : DbConnection, ICloneable
{
	public override string ConnectionString
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public override string Database
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override string DataSource
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Provider
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override string ServerVersion
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override ConnectionState State
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ConnectionState);
		}
	}

	public event OleDbInfoMessageEventHandler InfoMessage
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

	public OleDbConnection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbConnection(string connectionString)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override DbTransaction BeginDbTransaction(IsolationLevel isolationLevel)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void ChangeDatabase(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Close()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override DbCommand CreateDbCommand()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void EnlistDistributedTransaction(ITransaction transaction)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public DataTable GetOleDbSchemaTable(Guid schema, object[] restrictions)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void Open()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void ReleaseObjectPool()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	public void ResetState()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	object ICloneable.Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

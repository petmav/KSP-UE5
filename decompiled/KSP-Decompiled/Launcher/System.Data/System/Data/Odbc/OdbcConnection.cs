using System.ComponentModel;
using System.Data.Common;
using System.EnterpriseServices;
using Unity;

namespace System.Data.Odbc;

[DefaultEvent("InfoMessage")]
public sealed class OdbcConnection : DbConnection, ICloneable
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

	public new int ConnectionTimeout
	{
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

	public string Driver
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

	public event OdbcInfoMessageEventHandler InfoMessage
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

	public OdbcConnection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcConnection(string connectionString)
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

	public override void Open()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void ReleaseObjectPool()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	object ICloneable.Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

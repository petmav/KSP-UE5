using System.ComponentModel;
using System.Data.Common;
using Unity;

namespace System.Data.OleDb;

[DefaultEvent("RecordsAffected")]
[Designer("Microsoft.VSDesigner.Data.VS.OleDbCommandDesigner, Microsoft.VSDesigner, Version=10.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
[ToolboxItem(true)]
public sealed class OleDbCommand : DbCommand, ICloneable
{
	public override string CommandText
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

	public override int CommandTimeout
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public override CommandType CommandType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(CommandType);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public new OleDbConnection Connection
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected override DbConnection DbConnection
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

	protected override DbParameterCollection DbParameterCollection
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected override DbTransaction DbTransaction
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

	public override bool DesignTimeVisible
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public new OleDbTransaction Transaction
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public override UpdateRowSource UpdatedRowSource
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(UpdateRowSource);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public OleDbCommand()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbCommand(string cmdText)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbCommand(string cmdText, OleDbConnection connection)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbCommand(string cmdText, OleDbConnection connection, OleDbTransaction transaction)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Cancel()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbCommand Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override DbParameter CreateDbParameter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override DbDataReader ExecuteDbDataReader(CommandBehavior behavior)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override int ExecuteNonQuery()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public override object ExecuteScalar()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void Prepare()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ResetCommandTimeout()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	object ICloneable.Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

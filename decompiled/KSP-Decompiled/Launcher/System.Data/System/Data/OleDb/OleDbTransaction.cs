using System.Data.Common;
using Unity;

namespace System.Data.OleDb;

public sealed class OleDbTransaction : DbTransaction
{
	protected override DbConnection DbConnection
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override IsolationLevel IsolationLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(IsolationLevel);
		}
	}

	internal OleDbTransaction()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbTransaction Begin()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public OleDbTransaction Begin(IsolationLevel isolevel)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void Commit()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Rollback()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

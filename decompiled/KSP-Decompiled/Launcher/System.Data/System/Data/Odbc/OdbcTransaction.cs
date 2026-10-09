using System.Data.Common;
using Unity;

namespace System.Data.Odbc;

public sealed class OdbcTransaction : DbTransaction
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

	internal OdbcTransaction()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
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

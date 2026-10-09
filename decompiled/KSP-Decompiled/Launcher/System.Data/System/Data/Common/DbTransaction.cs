namespace System.Data.Common;

public abstract class DbTransaction : MarshalByRefObject, IDbTransaction, IDisposable
{
	public DbConnection Connection => DbConnection;

	IDbConnection IDbTransaction.Connection => DbConnection;

	protected abstract DbConnection DbConnection { get; }

	public abstract IsolationLevel IsolationLevel { get; }

	public abstract void Commit();

	public void Dispose()
	{
		Dispose(disposing: true);
	}

	protected virtual void Dispose(bool disposing)
	{
	}

	public abstract void Rollback();
}

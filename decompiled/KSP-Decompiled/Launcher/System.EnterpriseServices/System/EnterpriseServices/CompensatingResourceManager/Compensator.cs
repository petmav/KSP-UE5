using Unity;

namespace System.EnterpriseServices.CompensatingResourceManager;

public class Compensator : ServicedComponent
{
	public Clerk Clerk
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Compensator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual bool AbortRecord(LogRecord rec)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual void BeginAbort(bool fRecovery)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void BeginCommit(bool fRecovery)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void BeginPrepare()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual bool CommitRecord(LogRecord rec)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual void EndAbort()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void EndCommit()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual bool EndPrepare()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool PrepareRecord(LogRecord rec)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}

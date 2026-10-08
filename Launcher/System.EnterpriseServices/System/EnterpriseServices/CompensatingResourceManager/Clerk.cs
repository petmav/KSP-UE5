using Unity;

namespace System.EnterpriseServices.CompensatingResourceManager;

public sealed class Clerk
{
	public int LogRecordCount
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public string TransactionUOW
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Clerk(string compensator, string description, CompensatorOptions flags)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Clerk(Type compensator, string description, CompensatorOptions flags)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ForceLog()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ForceTransactionToAbort()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ForgetLogRecord()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void WriteLogRecord(object record)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

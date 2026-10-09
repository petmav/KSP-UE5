using Unity;

namespace System.EnterpriseServices.CompensatingResourceManager;

public sealed class LogRecord
{
	public LogRecordFlags Flags
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(LogRecordFlags);
		}
	}

	public object Record
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public int Sequence
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	internal LogRecord()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

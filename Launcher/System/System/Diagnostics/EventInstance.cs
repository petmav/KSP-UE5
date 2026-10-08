using Unity;

namespace System.Diagnostics;

public class EventInstance
{
	public int CategoryId
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

	public EventLogEntryType EntryType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(EventLogEntryType);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public long InstanceId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public EventInstance(long instanceId, int categoryId)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventInstance(long instanceId, int categoryId, EventLogEntryType entryType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

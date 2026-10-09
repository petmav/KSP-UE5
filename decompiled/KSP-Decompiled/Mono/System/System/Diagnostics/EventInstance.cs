using System.ComponentModel;

namespace System.Diagnostics;

public class EventInstance
{
	private int _categoryId;

	private EventLogEntryType _entryType;

	private long _instanceId;

	public int CategoryId
	{
		get
		{
			return _categoryId;
		}
		set
		{
			if (value < 0 || value > 65535)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			_categoryId = value;
		}
	}

	public EventLogEntryType EntryType
	{
		get
		{
			return _entryType;
		}
		set
		{
			if (!Enum.IsDefined(typeof(EventLogEntryType), value))
			{
				throw new InvalidEnumArgumentException("value", (int)value, typeof(EventLogEntryType));
			}
			_entryType = value;
		}
	}

	public long InstanceId
	{
		get
		{
			return _instanceId;
		}
		set
		{
			if (value < 0 || value > uint.MaxValue)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			_instanceId = value;
		}
	}

	public EventInstance(long instanceId, int categoryId)
		: this(instanceId, categoryId, EventLogEntryType.Information)
	{
	}

	public EventInstance(long instanceId, int categoryId, EventLogEntryType entryType)
	{
		InstanceId = instanceId;
		CategoryId = categoryId;
		EntryType = entryType;
	}
}

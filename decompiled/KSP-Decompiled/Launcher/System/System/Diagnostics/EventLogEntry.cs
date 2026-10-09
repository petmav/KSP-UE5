using System.ComponentModel;
using System.Runtime.Serialization;
using Unity;

namespace System.Diagnostics;

[Serializable]
[DesignTimeVisible(false)]
[ToolboxItem(false)]
public sealed class EventLogEntry : Component, ISerializable
{
	public string Category
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public short CategoryNumber
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(short);
		}
	}

	public byte[] Data
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public EventLogEntryType EntryType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(EventLogEntryType);
		}
	}

	public int EventID
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public int Index
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public long InstanceId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public string MachineName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Message
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string[] ReplacementStrings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string Source
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public DateTime TimeGenerated
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(DateTime);
		}
	}

	public DateTime TimeWritten
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(DateTime);
		}
	}

	public string UserName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal EventLogEntry()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Equals(EventLogEntry otherEntry)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

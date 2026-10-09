using Unity;

namespace System.Diagnostics;

public class EntryWrittenEventArgs : EventArgs
{
	public EventLogEntry Entry
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public EntryWrittenEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EntryWrittenEventArgs(EventLogEntry entry)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

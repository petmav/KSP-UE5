using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing.Reader;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventLogWatcher : IDisposable
{
	public bool Enabled
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event EventHandler<EventRecordWrittenEventArgs> EventRecordWritten
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public EventLogWatcher(EventLogQuery eventQuery)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogWatcher(EventLogQuery eventQuery, EventBookmark bookmark)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogWatcher(EventLogQuery eventQuery, EventBookmark bookmark, bool readExistingEvents)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogWatcher(string path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	protected virtual void Dispose(bool disposing)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

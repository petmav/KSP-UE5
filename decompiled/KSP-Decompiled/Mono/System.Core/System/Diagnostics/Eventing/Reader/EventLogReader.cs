using System.Collections.Generic;
using System.IO;
using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing.Reader;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventLogReader : IDisposable
{
	public int BatchSize
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

	public IList<EventLogStatus> LogStatus
	{
		[SecurityCritical]
		get
		{
			//IL_0007: Expected O, but got I4
			Unity.ThrowStub.ThrowNotSupportedException();
			return (IList<EventLogStatus>)0;
		}
	}

	public EventLogReader(EventLogQuery eventQuery)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	public EventLogReader(EventLogQuery eventQuery, EventBookmark bookmark)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogReader(string path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventLogReader(string path, PathType pathType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CancelReading()
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

	public EventRecord ReadEvent()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecurityCritical]
	public EventRecord ReadEvent(TimeSpan timeout)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Seek(EventBookmark bookmark)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	public void Seek(EventBookmark bookmark, long offset)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	public void Seek(SeekOrigin origin, long offset)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.Eventing;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventProvider : IDisposable
{
	public enum WriteEventErrorCode
	{
		EventTooBig = 2,
		NoError = 0,
		NoFreeBuffers = 1
	}

	[SecuritySafeCritical]
	[PermissionSet(SecurityAction.Demand, Unrestricted = true)]
	public EventProvider(Guid providerGuid)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void Close()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	public static Guid CreateActivityId()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(Guid);
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

	public static WriteEventErrorCode GetLastWriteEventError()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(WriteEventErrorCode);
	}

	public bool IsEnabled()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsEnabled(byte level, long keywords)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SecurityCritical]
	public static void SetActivityId(ref Guid id)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	protected bool WriteEvent(ref EventDescriptor eventDescriptor, int dataCount, IntPtr data)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool WriteEvent(ref EventDescriptor eventDescriptor, object[] eventPayload)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SecurityCritical]
	public bool WriteEvent(ref EventDescriptor eventDescriptor, string data)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool WriteMessageEvent(string eventMessage)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SecurityCritical]
	public bool WriteMessageEvent(string eventMessage, byte eventLevel, long eventKeywords)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SecurityCritical]
	protected bool WriteTransferEvent(ref EventDescriptor eventDescriptor, Guid relatedActivityId, int dataCount, IntPtr data)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SecurityCritical]
	public bool WriteTransferEvent(ref EventDescriptor eventDescriptor, Guid relatedActivityId, object[] eventPayload)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}

using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.PerformanceData;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class CounterData
{
	public long RawValue
	{
		[SecurityCritical]
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
		[SecurityCritical]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public long Value
	{
		[SecurityCritical]
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
		[SecurityCritical]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	internal CounterData()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	public void Decrement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	public void Increment()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	public void IncrementBy(long value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

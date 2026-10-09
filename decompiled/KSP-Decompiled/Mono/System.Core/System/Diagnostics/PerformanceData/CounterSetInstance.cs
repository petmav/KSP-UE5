using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.PerformanceData;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class CounterSetInstance : IDisposable
{
	public CounterSetInstanceCounterDataSet Counters
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal CounterSetInstance()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityCritical]
	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

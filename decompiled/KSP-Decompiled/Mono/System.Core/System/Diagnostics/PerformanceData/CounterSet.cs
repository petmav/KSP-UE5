using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.PerformanceData;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class CounterSet : IDisposable
{
	[SecuritySafeCritical]
	[PermissionSet(SecurityAction.Demand, Unrestricted = true)]
	public CounterSet(Guid providerGuid, Guid counterSetGuid, CounterSetInstanceType instanceType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddCounter(int counterId, CounterType counterType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddCounter(int counterId, CounterType counterType, string counterName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	[PermissionSet(SecurityAction.Demand, Unrestricted = true)]
	public CounterSetInstance CreateCounterSetInstance(string instanceName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
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

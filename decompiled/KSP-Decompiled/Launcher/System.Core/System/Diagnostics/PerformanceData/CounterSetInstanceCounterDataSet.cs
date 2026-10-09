using System.Runtime.CompilerServices;
using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics.PerformanceData;

[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class CounterSetInstanceCounterDataSet : IDisposable
{
	public CounterData this[string counterName]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal CounterSetInstanceCounterDataSet()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public CounterData get_Item(int counterId)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecurityCritical]
	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

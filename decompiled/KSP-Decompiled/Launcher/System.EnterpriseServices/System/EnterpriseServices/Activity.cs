using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[ComVisible(false)]
public sealed class Activity
{
	public Activity(ServiceConfig cfg)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AsynchronousCall(IServiceCall serviceCall)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void BindToCurrentThread()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SynchronousCall(IServiceCall serviceCall)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void UnbindFromThread()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

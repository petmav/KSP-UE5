using Unity;

namespace System.EnterpriseServices;

[Serializable]
public abstract class ServicedComponent : ContextBoundObject, IRemoteDispatch, IServicedComponentInfo, IDisposable
{
	public ServicedComponent()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual void Activate()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual bool CanBePooled()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal virtual void Construct(string s)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal virtual void Deactivate()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void Dispose(bool disposing)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void DisposeObject(ServicedComponent sc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[AutoComplete(true)]
	string IRemoteDispatch.RemoteDispatchAutoDone(string s)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[AutoComplete(false)]
	string IRemoteDispatch.RemoteDispatchNotAutoDone(string s)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	void IServicedComponentInfo.GetComponentInfo(ref int infoMask, out string[] infoArray)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

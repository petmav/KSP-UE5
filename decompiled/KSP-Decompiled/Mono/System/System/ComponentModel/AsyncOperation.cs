using System.Security.Permissions;
using System.Threading;
using Unity;

namespace System.ComponentModel;

[HostProtection(SecurityAction.LinkDemand, SharedState = true)]
public sealed class AsyncOperation
{
	private SynchronizationContext syncContext;

	private object userSuppliedState;

	private bool alreadyCompleted;

	public object UserSuppliedState => userSuppliedState;

	public SynchronizationContext SynchronizationContext => syncContext;

	private AsyncOperation(object userSuppliedState, SynchronizationContext syncContext)
	{
		this.userSuppliedState = userSuppliedState;
		this.syncContext = syncContext;
		alreadyCompleted = false;
		this.syncContext.OperationStarted();
	}

	~AsyncOperation()
	{
		if (!alreadyCompleted && syncContext != null)
		{
			syncContext.OperationCompleted();
		}
	}

	public void Post(SendOrPostCallback d, object arg)
	{
		VerifyNotCompleted();
		VerifyDelegateNotNull(d);
		syncContext.Post(d, arg);
	}

	public void PostOperationCompleted(SendOrPostCallback d, object arg)
	{
		Post(d, arg);
		OperationCompletedCore();
	}

	public void OperationCompleted()
	{
		VerifyNotCompleted();
		OperationCompletedCore();
	}

	private void OperationCompletedCore()
	{
		try
		{
			syncContext.OperationCompleted();
		}
		finally
		{
			alreadyCompleted = true;
			GC.SuppressFinalize(this);
		}
	}

	private void VerifyNotCompleted()
	{
		if (alreadyCompleted)
		{
			throw new InvalidOperationException(global::SR.GetString("This operation has already had OperationCompleted called on it and further calls are illegal."));
		}
	}

	private void VerifyDelegateNotNull(SendOrPostCallback d)
	{
		if (d == null)
		{
			throw new ArgumentNullException(global::SR.GetString("A non-null SendOrPostCallback must be supplied."), "d");
		}
	}

	internal static AsyncOperation CreateOperation(object userSuppliedState, SynchronizationContext syncContext)
	{
		return new AsyncOperation(userSuppliedState, syncContext);
	}

	internal AsyncOperation()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

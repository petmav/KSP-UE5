using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;

namespace System.Threading;

[ComVisible(false)]
[DebuggerDisplay("IsCancellationRequested = {IsCancellationRequested}")]
[HostProtection(SecurityAction.LinkDemand, Synchronization = true, ExternalThreading = true)]
public struct CancellationToken
{
	private CancellationTokenSource m_source;

	private static readonly Action<object> s_ActionToActionObjShunt = ActionToActionObjShunt;

	public static CancellationToken None => default(CancellationToken);

	public bool IsCancellationRequested
	{
		get
		{
			if (m_source != null)
			{
				return m_source.IsCancellationRequested;
			}
			return false;
		}
	}

	public bool CanBeCanceled
	{
		get
		{
			if (m_source != null)
			{
				return m_source.CanBeCanceled;
			}
			return false;
		}
	}

	public WaitHandle WaitHandle
	{
		get
		{
			if (m_source == null)
			{
				InitializeDefaultSource();
			}
			return m_source.WaitHandle;
		}
	}

	internal CancellationToken(CancellationTokenSource source)
	{
		m_source = source;
	}

	public CancellationToken(bool canceled)
	{
		this = default(CancellationToken);
		if (canceled)
		{
			m_source = CancellationTokenSource.InternalGetStaticSource(canceled);
		}
	}

	private static void ActionToActionObjShunt(object obj)
	{
		(obj as Action)();
	}

	public CancellationTokenRegistration Register(Action callback)
	{
		if (callback == null)
		{
			throw new ArgumentNullException("callback");
		}
		return Register(s_ActionToActionObjShunt, callback, useSynchronizationContext: false, useExecutionContext: true);
	}

	public CancellationTokenRegistration Register(Action callback, bool useSynchronizationContext)
	{
		if (callback == null)
		{
			throw new ArgumentNullException("callback");
		}
		return Register(s_ActionToActionObjShunt, callback, useSynchronizationContext, useExecutionContext: true);
	}

	public CancellationTokenRegistration Register(Action<object> callback, object state)
	{
		if (callback == null)
		{
			throw new ArgumentNullException("callback");
		}
		return Register(callback, state, useSynchronizationContext: false, useExecutionContext: true);
	}

	public CancellationTokenRegistration Register(Action<object> callback, object state, bool useSynchronizationContext)
	{
		return Register(callback, state, useSynchronizationContext, useExecutionContext: true);
	}

	internal CancellationTokenRegistration InternalRegisterWithoutEC(Action<object> callback, object state)
	{
		return Register(callback, state, useSynchronizationContext: false, useExecutionContext: false);
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	[SecuritySafeCritical]
	private CancellationTokenRegistration Register(Action<object> callback, object state, bool useSynchronizationContext, bool useExecutionContext)
	{
		StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
		if (callback == null)
		{
			throw new ArgumentNullException("callback");
		}
		if (!CanBeCanceled)
		{
			return default(CancellationTokenRegistration);
		}
		SynchronizationContext targetSyncContext = null;
		ExecutionContext executionContext = null;
		if (!IsCancellationRequested)
		{
			if (useSynchronizationContext)
			{
				targetSyncContext = SynchronizationContext.Current;
			}
			if (useExecutionContext)
			{
				executionContext = ExecutionContext.Capture(ref stackMark, ExecutionContext.CaptureOptions.OptimizeDefaultCase);
			}
		}
		return m_source.InternalRegister(callback, state, targetSyncContext, executionContext);
	}

	public bool Equals(CancellationToken other)
	{
		if (m_source == null && other.m_source == null)
		{
			return true;
		}
		if (m_source == null)
		{
			return other.m_source == CancellationTokenSource.InternalGetStaticSource(set: false);
		}
		if (other.m_source == null)
		{
			return m_source == CancellationTokenSource.InternalGetStaticSource(set: false);
		}
		return m_source == other.m_source;
	}

	public override bool Equals(object other)
	{
		if (other is CancellationToken)
		{
			return Equals((CancellationToken)other);
		}
		return false;
	}

	public override int GetHashCode()
	{
		if (m_source == null)
		{
			return CancellationTokenSource.InternalGetStaticSource(set: false).GetHashCode();
		}
		return m_source.GetHashCode();
	}

	public static bool operator ==(CancellationToken left, CancellationToken right)
	{
		return left.Equals(right);
	}

	public static bool operator !=(CancellationToken left, CancellationToken right)
	{
		return !left.Equals(right);
	}

	public void ThrowIfCancellationRequested()
	{
		if (IsCancellationRequested)
		{
			ThrowOperationCanceledException();
		}
	}

	internal void ThrowIfSourceDisposed()
	{
		if (m_source != null && m_source.IsDisposed)
		{
			ThrowObjectDisposedException();
		}
	}

	private void ThrowOperationCanceledException()
	{
		throw new OperationCanceledException(Environment.GetResourceString("The operation was canceled."), this);
	}

	private static void ThrowObjectDisposedException()
	{
		throw new ObjectDisposedException(null, Environment.GetResourceString("The CancellationTokenSource associated with this CancellationToken has been disposed."));
	}

	private void InitializeDefaultSource()
	{
		m_source = CancellationTokenSource.InternalGetStaticSource(set: false);
	}
}

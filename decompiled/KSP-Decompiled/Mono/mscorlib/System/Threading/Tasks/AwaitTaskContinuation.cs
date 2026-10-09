using System.Runtime.CompilerServices;
using System.Runtime.ExceptionServices;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Security;

namespace System.Threading.Tasks;

internal class AwaitTaskContinuation : TaskContinuation, IThreadPoolWorkItem
{
	private readonly ExecutionContext m_capturedContext;

	protected readonly Action m_action;

	[SecurityCritical]
	private static ContextCallback s_invokeActionCallback;

	internal static bool IsValidLocationForInlining
	{
		get
		{
			SynchronizationContext currentNoFlow = SynchronizationContext.CurrentNoFlow;
			if (currentNoFlow != null && currentNoFlow.GetType() != typeof(SynchronizationContext))
			{
				return false;
			}
			TaskScheduler internalCurrent = TaskScheduler.InternalCurrent;
			if (internalCurrent != null)
			{
				return internalCurrent == TaskScheduler.Default;
			}
			return true;
		}
	}

	[SecurityCritical]
	internal AwaitTaskContinuation(Action action, bool flowExecutionContext, ref StackCrawlMark stackMark)
	{
		m_action = action;
		if (flowExecutionContext)
		{
			m_capturedContext = ExecutionContext.Capture(ref stackMark, ExecutionContext.CaptureOptions.IgnoreSyncCtx | ExecutionContext.CaptureOptions.OptimizeDefaultCase);
		}
	}

	[SecurityCritical]
	internal AwaitTaskContinuation(Action action, bool flowExecutionContext)
	{
		m_action = action;
		if (flowExecutionContext)
		{
			m_capturedContext = ExecutionContext.FastCapture();
		}
	}

	protected Task CreateTask(Action<object> action, object state, TaskScheduler scheduler)
	{
		return new Task(action, state, null, default(CancellationToken), TaskCreationOptions.None, InternalTaskOptions.QueuedByRuntime, scheduler)
		{
			CapturedContext = m_capturedContext
		};
	}

	[SecuritySafeCritical]
	internal override void Run(Task task, bool canInlineContinuationTask)
	{
		if (canInlineContinuationTask && IsValidLocationForInlining)
		{
			RunCallback(GetInvokeActionCallback(), m_action, ref Task.t_currentTask);
		}
		else
		{
			ThreadPool.UnsafeQueueCustomWorkItem(this, forceGlobal: false);
		}
	}

	[SecurityCritical]
	private void ExecuteWorkItemHelper()
	{
		if (m_capturedContext == null)
		{
			m_action();
			return;
		}
		try
		{
			ExecutionContext.Run(m_capturedContext, GetInvokeActionCallback(), m_action, preserveSyncCtx: true);
		}
		finally
		{
			m_capturedContext.Dispose();
		}
	}

	[SecurityCritical]
	void IThreadPoolWorkItem.ExecuteWorkItem()
	{
		if (m_capturedContext == null)
		{
			m_action();
		}
		else
		{
			ExecuteWorkItemHelper();
		}
	}

	[SecurityCritical]
	void IThreadPoolWorkItem.MarkAborted(ThreadAbortException tae)
	{
	}

	[SecurityCritical]
	private static void InvokeAction(object state)
	{
		((Action)state)();
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	[SecurityCritical]
	protected static ContextCallback GetInvokeActionCallback()
	{
		return InvokeAction;
	}

	[SecurityCritical]
	protected void RunCallback(ContextCallback callback, object state, ref Task currentTask)
	{
		Task task = currentTask;
		try
		{
			if (task != null)
			{
				currentTask = null;
			}
			if (m_capturedContext == null)
			{
				callback(state);
			}
			else
			{
				ExecutionContext.Run(m_capturedContext, callback, state, preserveSyncCtx: true);
			}
		}
		catch (Exception exc)
		{
			ThrowAsyncIfNecessary(exc);
		}
		finally
		{
			if (task != null)
			{
				currentTask = task;
			}
			if (m_capturedContext != null)
			{
				m_capturedContext.Dispose();
			}
		}
	}

	[SecurityCritical]
	internal static void RunOrScheduleAction(Action action, bool allowInlining, ref Task currentTask)
	{
		if (!allowInlining || !IsValidLocationForInlining)
		{
			UnsafeScheduleAction(action, currentTask);
			return;
		}
		Task task = currentTask;
		try
		{
			if (task != null)
			{
				currentTask = null;
			}
			action();
		}
		catch (Exception exc)
		{
			ThrowAsyncIfNecessary(exc);
		}
		finally
		{
			if (task != null)
			{
				currentTask = task;
			}
		}
	}

	[SecurityCritical]
	internal static void UnsafeScheduleAction(Action action, Task task)
	{
		ThreadPool.UnsafeQueueCustomWorkItem(new AwaitTaskContinuation(action, flowExecutionContext: false), forceGlobal: false);
	}

	protected static void ThrowAsyncIfNecessary(Exception exc)
	{
		if (!(exc is ThreadAbortException) && !(exc is AppDomainUnloadedException) && !WindowsRuntimeMarshal.ReportUnhandledError(exc))
		{
			ExceptionDispatchInfo state = ExceptionDispatchInfo.Capture(exc);
			ThreadPool.QueueUserWorkItem(delegate(object s)
			{
				((ExceptionDispatchInfo)s).Throw();
			}, state);
		}
	}

	internal override Delegate[] GetDelegateContinuationsForDebugger()
	{
		return new Delegate[1] { AsyncMethodBuilderCore.TryGetStateMachineForDebugger(m_action) };
	}
}

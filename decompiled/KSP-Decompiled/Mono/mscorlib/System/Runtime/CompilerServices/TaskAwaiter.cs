using System.Collections.ObjectModel;
using System.Runtime.ExceptionServices;
using System.Security;
using System.Security.Permissions;
using System.Threading;
using System.Threading.Tasks;

namespace System.Runtime.CompilerServices;

[HostProtection(SecurityAction.LinkDemand, Synchronization = true, ExternalThreading = true)]
public struct TaskAwaiter : ICriticalNotifyCompletion, INotifyCompletion
{
	private readonly Task m_task;

	public bool IsCompleted => m_task.IsCompleted;

	internal TaskAwaiter(Task task)
	{
		m_task = task;
	}

	[SecuritySafeCritical]
	public void OnCompleted(Action continuation)
	{
		OnCompletedInternal(m_task, continuation, continueOnCapturedContext: true, flowExecutionContext: true);
	}

	[SecurityCritical]
	public void UnsafeOnCompleted(Action continuation)
	{
		OnCompletedInternal(m_task, continuation, continueOnCapturedContext: true, flowExecutionContext: false);
	}

	public void GetResult()
	{
		ValidateEnd(m_task);
	}

	internal static void ValidateEnd(Task task)
	{
		if (task.IsWaitNotificationEnabledOrNotRanToCompletion)
		{
			HandleNonSuccessAndDebuggerNotification(task);
		}
	}

	private static void HandleNonSuccessAndDebuggerNotification(Task task)
	{
		if (!task.IsCompleted)
		{
			task.InternalWait(-1, default(CancellationToken));
		}
		task.NotifyDebuggerOfWaitCompletionIfNecessary();
		if (!task.IsRanToCompletion)
		{
			ThrowForNonSuccess(task);
		}
	}

	private static void ThrowForNonSuccess(Task task)
	{
		switch (task.Status)
		{
		case TaskStatus.Canceled:
			task.GetCancellationExceptionDispatchInfo()?.Throw();
			throw new TaskCanceledException(task);
		case TaskStatus.Faulted:
		{
			ReadOnlyCollection<ExceptionDispatchInfo> exceptionDispatchInfos = task.GetExceptionDispatchInfos();
			if (exceptionDispatchInfos.Count > 0)
			{
				exceptionDispatchInfos[0].Throw();
				break;
			}
			throw task.Exception;
		}
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	[SecurityCritical]
	internal static void OnCompletedInternal(Task task, Action continuation, bool continueOnCapturedContext, bool flowExecutionContext)
	{
		if (continuation == null)
		{
			throw new ArgumentNullException("continuation");
		}
		StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
		task.SetContinuationForAwait(continuation, continueOnCapturedContext, flowExecutionContext, ref stackMark);
	}

	private static Action OutputWaitEtwEvents(Task task, Action continuation)
	{
		if (Task.s_asyncDebuggingEnabled)
		{
			Task.AddToActiveTasks(task);
		}
		return AsyncMethodBuilderCore.CreateContinuationWrapper(continuation, delegate
		{
			if (Task.s_asyncDebuggingEnabled)
			{
				Task.RemoveFromActiveTasks(task.Id);
			}
			continuation();
		});
	}
}
[HostProtection(SecurityAction.LinkDemand, Synchronization = true, ExternalThreading = true)]
public struct TaskAwaiter<TResult> : ICriticalNotifyCompletion, INotifyCompletion
{
	private readonly Task<TResult> m_task;

	public bool IsCompleted => m_task.IsCompleted;

	internal TaskAwaiter(Task<TResult> task)
	{
		m_task = task;
	}

	[SecuritySafeCritical]
	public void OnCompleted(Action continuation)
	{
		TaskAwaiter.OnCompletedInternal(m_task, continuation, continueOnCapturedContext: true, flowExecutionContext: true);
	}

	[SecurityCritical]
	public void UnsafeOnCompleted(Action continuation)
	{
		TaskAwaiter.OnCompletedInternal(m_task, continuation, continueOnCapturedContext: true, flowExecutionContext: false);
	}

	public TResult GetResult()
	{
		TaskAwaiter.ValidateEnd(m_task);
		return m_task.ResultOnSuccess;
	}
}

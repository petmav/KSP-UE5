using System.Runtime.CompilerServices;
using System.Security;

namespace System.Threading.Tasks;

internal sealed class SynchronizationContextAwaitTaskContinuation : AwaitTaskContinuation
{
	private static readonly SendOrPostCallback s_postCallback = delegate(object state)
	{
		((Action)state)();
	};

	[SecurityCritical]
	private static ContextCallback s_postActionCallback;

	private readonly SynchronizationContext m_syncContext;

	[SecurityCritical]
	internal SynchronizationContextAwaitTaskContinuation(SynchronizationContext context, Action action, bool flowExecutionContext, ref StackCrawlMark stackMark)
		: base(action, flowExecutionContext, ref stackMark)
	{
		m_syncContext = context;
	}

	[SecuritySafeCritical]
	internal sealed override void Run(Task task, bool canInlineContinuationTask)
	{
		if (canInlineContinuationTask && m_syncContext == SynchronizationContext.CurrentNoFlow)
		{
			RunCallback(AwaitTaskContinuation.GetInvokeActionCallback(), m_action, ref Task.t_currentTask);
		}
		else
		{
			RunCallback(GetPostActionCallback(), this, ref Task.t_currentTask);
		}
	}

	[SecurityCritical]
	private static void PostAction(object state)
	{
		SynchronizationContextAwaitTaskContinuation synchronizationContextAwaitTaskContinuation = (SynchronizationContextAwaitTaskContinuation)state;
		synchronizationContextAwaitTaskContinuation.m_syncContext.Post(s_postCallback, synchronizationContextAwaitTaskContinuation.m_action);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	[SecurityCritical]
	private static ContextCallback GetPostActionCallback()
	{
		return PostAction;
	}
}

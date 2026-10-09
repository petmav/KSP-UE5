using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Security;
using System.Security.Permissions;

namespace System.Threading.Tasks;

[DebuggerTypeProxy(typeof(SystemThreadingTasks_TaskSchedulerDebugView))]
[DebuggerDisplay("Id={Id}")]
[PermissionSet(SecurityAction.InheritanceDemand, Unrestricted = true)]
[HostProtection(SecurityAction.LinkDemand, Synchronization = true, ExternalThreading = true)]
public abstract class TaskScheduler
{
	internal sealed class SystemThreadingTasks_TaskSchedulerDebugView
	{
		private readonly TaskScheduler m_taskScheduler;

		public int Id => m_taskScheduler.Id;

		public IEnumerable<Task> ScheduledTasks
		{
			[SecurityCritical]
			get
			{
				return m_taskScheduler.GetScheduledTasks();
			}
		}

		public SystemThreadingTasks_TaskSchedulerDebugView(TaskScheduler scheduler)
		{
			m_taskScheduler = scheduler;
		}
	}

	private static ConditionalWeakTable<TaskScheduler, object> s_activeTaskSchedulers;

	private static readonly TaskScheduler s_defaultTaskScheduler = new ThreadPoolTaskScheduler();

	internal static int s_taskSchedulerIdCounter;

	private volatile int m_taskSchedulerId;

	private static EventHandler<UnobservedTaskExceptionEventArgs> _unobservedTaskException;

	private static readonly object _unobservedTaskExceptionLockObject = new object();

	public virtual int MaximumConcurrencyLevel => int.MaxValue;

	internal virtual bool RequiresAtomicStartTransition => true;

	public static TaskScheduler Default => s_defaultTaskScheduler;

	public static TaskScheduler Current => InternalCurrent ?? Default;

	internal static bool IsDefault => Current == Default;

	internal static TaskScheduler InternalCurrent
	{
		get
		{
			Task internalCurrent = Task.InternalCurrent;
			if (internalCurrent == null || (internalCurrent.CreationOptions & TaskCreationOptions.HideScheduler) != 0)
			{
				return null;
			}
			return internalCurrent.ExecutingTaskScheduler;
		}
	}

	public int Id
	{
		get
		{
			if (m_taskSchedulerId == 0)
			{
				int num = 0;
				do
				{
					num = Interlocked.Increment(ref s_taskSchedulerIdCounter);
				}
				while (num == 0);
				Interlocked.CompareExchange(ref m_taskSchedulerId, num, 0);
			}
			return m_taskSchedulerId;
		}
	}

	public static event EventHandler<UnobservedTaskExceptionEventArgs> UnobservedTaskException
	{
		[SecurityCritical]
		add
		{
			if (value != null)
			{
				RuntimeHelpers.PrepareContractedDelegate(value);
				lock (_unobservedTaskExceptionLockObject)
				{
					_unobservedTaskException = (EventHandler<UnobservedTaskExceptionEventArgs>)Delegate.Combine(_unobservedTaskException, value);
				}
			}
		}
		[SecurityCritical]
		remove
		{
			lock (_unobservedTaskExceptionLockObject)
			{
				_unobservedTaskException = (EventHandler<UnobservedTaskExceptionEventArgs>)Delegate.Remove(_unobservedTaskException, value);
			}
		}
	}

	[SecurityCritical]
	protected internal abstract void QueueTask(Task task);

	[SecurityCritical]
	protected abstract bool TryExecuteTaskInline(Task task, bool taskWasPreviouslyQueued);

	[SecurityCritical]
	protected abstract IEnumerable<Task> GetScheduledTasks();

	[SecuritySafeCritical]
	internal bool TryRunInline(Task task, bool taskWasPreviouslyQueued)
	{
		TaskScheduler executingTaskScheduler = task.ExecutingTaskScheduler;
		if (executingTaskScheduler != this && executingTaskScheduler != null)
		{
			return executingTaskScheduler.TryRunInline(task, taskWasPreviouslyQueued);
		}
		StackGuard currentStackGuard;
		if (executingTaskScheduler == null || task.m_action == null || task.IsDelegateInvoked || task.IsCanceled || !(currentStackGuard = Task.CurrentStackGuard).TryBeginInliningScope())
		{
			return false;
		}
		bool flag = false;
		try
		{
			task.FireTaskScheduledIfNeeded(this);
			flag = TryExecuteTaskInline(task, taskWasPreviouslyQueued);
		}
		finally
		{
			currentStackGuard.EndInliningScope();
		}
		if (flag && !task.IsDelegateInvoked && !task.IsCanceled)
		{
			throw new InvalidOperationException(Environment.GetResourceString("The TryExecuteTaskInline call to the underlying scheduler succeeded, but the task body was not invoked."));
		}
		return flag;
	}

	[SecurityCritical]
	protected internal virtual bool TryDequeue(Task task)
	{
		return false;
	}

	internal virtual void NotifyWorkItemProgress()
	{
	}

	[SecurityCritical]
	internal void InternalQueueTask(Task task)
	{
		task.FireTaskScheduledIfNeeded(this);
		QueueTask(task);
	}

	protected TaskScheduler()
	{
		if (Debugger.IsAttached)
		{
			AddToActiveTaskSchedulers();
		}
	}

	private void AddToActiveTaskSchedulers()
	{
		ConditionalWeakTable<TaskScheduler, object> conditionalWeakTable = s_activeTaskSchedulers;
		if (conditionalWeakTable == null)
		{
			Interlocked.CompareExchange(ref s_activeTaskSchedulers, new ConditionalWeakTable<TaskScheduler, object>(), null);
			conditionalWeakTable = s_activeTaskSchedulers;
		}
		conditionalWeakTable.Add(this, null);
	}

	public static TaskScheduler FromCurrentSynchronizationContext()
	{
		return new SynchronizationContextTaskScheduler();
	}

	[SecurityCritical]
	protected bool TryExecuteTask(Task task)
	{
		if (task.ExecutingTaskScheduler != this)
		{
			throw new InvalidOperationException(Environment.GetResourceString("ExecuteTask may not be called for a task which was previously queued to a different TaskScheduler."));
		}
		return task.ExecuteEntry(bPreventDoubleExecution: true);
	}

	internal static void PublishUnobservedTaskException(object sender, UnobservedTaskExceptionEventArgs ueea)
	{
		lock (_unobservedTaskExceptionLockObject)
		{
			_unobservedTaskException?.Invoke(sender, ueea);
		}
	}

	[SecurityCritical]
	internal Task[] GetScheduledTasksForDebugger()
	{
		IEnumerable<Task> scheduledTasks = GetScheduledTasks();
		if (scheduledTasks == null)
		{
			return null;
		}
		Task[] array = scheduledTasks as Task[];
		if (array == null)
		{
			array = new List<Task>(scheduledTasks).ToArray();
		}
		Task[] array2 = array;
		for (int i = 0; i < array2.Length; i++)
		{
			_ = array2[i].Id;
		}
		return array;
	}

	[SecurityCritical]
	internal static TaskScheduler[] GetTaskSchedulersForDebugger()
	{
		if (s_activeTaskSchedulers == null)
		{
			return new TaskScheduler[1] { s_defaultTaskScheduler };
		}
		ICollection<TaskScheduler> keys = s_activeTaskSchedulers.Keys;
		if (!keys.Contains(s_defaultTaskScheduler))
		{
			keys.Add(s_defaultTaskScheduler);
		}
		TaskScheduler[] array = new TaskScheduler[keys.Count];
		keys.CopyTo(array, 0);
		TaskScheduler[] array2 = array;
		for (int i = 0; i < array2.Length; i++)
		{
			_ = array2[i].Id;
		}
		return array;
	}
}

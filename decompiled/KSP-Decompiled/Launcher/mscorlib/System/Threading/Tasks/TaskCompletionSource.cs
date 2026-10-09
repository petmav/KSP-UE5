using System.Collections.Generic;
using System.Runtime.ExceptionServices;
using System.Security.Permissions;

namespace System.Threading.Tasks;

[HostProtection(SecurityAction.LinkDemand, Synchronization = true, ExternalThreading = true)]
public class TaskCompletionSource<TResult>
{
	private readonly Task<TResult> m_task;

	public Task<TResult> Task => m_task;

	public TaskCompletionSource()
	{
		m_task = new Task<TResult>();
	}

	public TaskCompletionSource(TaskCreationOptions creationOptions)
		: this((object)null, creationOptions)
	{
	}

	public TaskCompletionSource(object state)
		: this(state, TaskCreationOptions.None)
	{
	}

	public TaskCompletionSource(object state, TaskCreationOptions creationOptions)
	{
		m_task = new Task<TResult>(state, creationOptions);
	}

	private void SpinUntilCompleted()
	{
		SpinWait spinWait = default(SpinWait);
		while (!m_task.IsCompleted)
		{
			spinWait.SpinOnce();
		}
	}

	public bool TrySetException(Exception exception)
	{
		if (exception == null)
		{
			throw new ArgumentNullException("exception");
		}
		bool num = m_task.TrySetException(exception);
		if (!num && !m_task.IsCompleted)
		{
			SpinUntilCompleted();
		}
		return num;
	}

	public bool TrySetException(IEnumerable<Exception> exceptions)
	{
		if (exceptions == null)
		{
			throw new ArgumentNullException("exceptions");
		}
		List<Exception> list = new List<Exception>();
		foreach (Exception exception in exceptions)
		{
			if (exception == null)
			{
				throw new ArgumentException(Environment.GetResourceString("The exceptions collection included at least one null element."), "exceptions");
			}
			list.Add(exception);
		}
		if (list.Count == 0)
		{
			throw new ArgumentException(Environment.GetResourceString("The exceptions collection was empty."), "exceptions");
		}
		bool num = m_task.TrySetException(list);
		if (!num && !m_task.IsCompleted)
		{
			SpinUntilCompleted();
		}
		return num;
	}

	internal bool TrySetException(IEnumerable<ExceptionDispatchInfo> exceptions)
	{
		bool num = m_task.TrySetException(exceptions);
		if (!num && !m_task.IsCompleted)
		{
			SpinUntilCompleted();
		}
		return num;
	}

	public void SetException(Exception exception)
	{
		if (exception == null)
		{
			throw new ArgumentNullException("exception");
		}
		if (!TrySetException(exception))
		{
			throw new InvalidOperationException(Environment.GetResourceString("An attempt was made to transition a task to a final state when it had already completed."));
		}
	}

	public void SetException(IEnumerable<Exception> exceptions)
	{
		if (!TrySetException(exceptions))
		{
			throw new InvalidOperationException(Environment.GetResourceString("An attempt was made to transition a task to a final state when it had already completed."));
		}
	}

	public bool TrySetResult(TResult result)
	{
		bool num = m_task.TrySetResult(result);
		if (!num && !m_task.IsCompleted)
		{
			SpinUntilCompleted();
		}
		return num;
	}

	public void SetResult(TResult result)
	{
		if (!TrySetResult(result))
		{
			throw new InvalidOperationException(Environment.GetResourceString("An attempt was made to transition a task to a final state when it had already completed."));
		}
	}

	public bool TrySetCanceled()
	{
		return TrySetCanceled(default(CancellationToken));
	}

	public bool TrySetCanceled(CancellationToken cancellationToken)
	{
		bool num = m_task.TrySetCanceled(cancellationToken);
		if (!num && !m_task.IsCompleted)
		{
			SpinUntilCompleted();
		}
		return num;
	}

	public void SetCanceled()
	{
		if (!TrySetCanceled())
		{
			throw new InvalidOperationException(Environment.GetResourceString("An attempt was made to transition a task to a final state when it had already completed."));
		}
	}
}

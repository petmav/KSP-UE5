using System.Runtime.Serialization;

namespace System.Threading.Tasks;

[Serializable]
public class TaskCanceledException : OperationCanceledException
{
	[NonSerialized]
	private Task m_canceledTask;

	public Task Task => m_canceledTask;

	public TaskCanceledException()
		: base(Environment.GetResourceString("A task was canceled."))
	{
	}

	public TaskCanceledException(string message)
		: base(message)
	{
	}

	public TaskCanceledException(string message, Exception innerException)
		: base(message, innerException)
	{
	}

	public TaskCanceledException(Task task)
		: base(Environment.GetResourceString("A task was canceled."), task?.CancellationToken ?? default(CancellationToken))
	{
		m_canceledTask = task;
	}

	protected TaskCanceledException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}

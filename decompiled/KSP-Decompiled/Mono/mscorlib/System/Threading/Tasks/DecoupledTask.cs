namespace System.Threading.Tasks;

internal sealed class DecoupledTask : IDecoupledTask
{
	public bool IsCompleted => Task.IsCompleted;

	public Task Task { get; private set; }

	public DecoupledTask(Task task)
	{
		Task = task;
	}
}
internal sealed class DecoupledTask<T> : IDecoupledTask
{
	public bool IsCompleted => Task.IsCompleted;

	public Task<T> Task { get; private set; }

	public DecoupledTask(Task<T> task)
	{
		Task = task;
	}
}

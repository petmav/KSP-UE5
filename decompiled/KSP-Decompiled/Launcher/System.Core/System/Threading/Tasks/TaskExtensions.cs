using System.Runtime.InteropServices;

namespace System.Threading.Tasks;

public static class TaskExtensions
{
	[StructLayout(LayoutKind.Sequential, Size = 1)]
	private struct VoidResult
	{
	}

	public static Task Unwrap(this Task<Task> task)
	{
		if (task == null)
		{
			throw new ArgumentNullException("task");
		}
		return Task.CreateUnwrapPromise<VoidResult>(task, lookForOce: false);
	}

	public static Task<TResult> Unwrap<TResult>(this Task<Task<TResult>> task)
	{
		if (task == null)
		{
			throw new ArgumentNullException("task");
		}
		return Task.CreateUnwrapPromise<TResult>(task, lookForOce: false);
	}
}

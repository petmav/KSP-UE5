using System.Threading.Tasks;

namespace System.Runtime.CompilerServices;

public struct ValueTaskAwaiter<TResult> : ICriticalNotifyCompletion, INotifyCompletion
{
	private readonly ValueTask<TResult> _value;

	public bool IsCompleted => _value.IsCompleted;

	internal ValueTaskAwaiter(ValueTask<TResult> value)
	{
		_value = value;
	}

	public TResult GetResult()
	{
		if (_value._task != null)
		{
			return _value._task.GetAwaiter().GetResult();
		}
		return _value._result;
	}

	public void OnCompleted(Action continuation)
	{
		_value.AsTask().ConfigureAwait(continueOnCapturedContext: true).GetAwaiter()
			.OnCompleted(continuation);
	}

	public void UnsafeOnCompleted(Action continuation)
	{
		_value.AsTask().ConfigureAwait(continueOnCapturedContext: true).GetAwaiter()
			.UnsafeOnCompleted(continuation);
	}
}

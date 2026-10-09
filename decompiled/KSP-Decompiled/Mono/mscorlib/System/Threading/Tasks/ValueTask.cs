using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace System.Threading.Tasks;

[StructLayout(LayoutKind.Auto)]
[AsyncMethodBuilder(typeof(AsyncValueTaskMethodBuilder<>))]
public struct ValueTask<TResult> : IEquatable<ValueTask<TResult>>
{
	internal readonly Task<TResult> _task;

	internal readonly TResult _result;

	public bool IsCompleted
	{
		get
		{
			if (_task != null)
			{
				return _task.IsCompleted;
			}
			return true;
		}
	}

	public bool IsCompletedSuccessfully
	{
		get
		{
			if (_task != null)
			{
				return _task.Status == TaskStatus.RanToCompletion;
			}
			return true;
		}
	}

	public bool IsFaulted
	{
		get
		{
			if (_task != null)
			{
				return _task.IsFaulted;
			}
			return false;
		}
	}

	public bool IsCanceled
	{
		get
		{
			if (_task != null)
			{
				return _task.IsCanceled;
			}
			return false;
		}
	}

	public TResult Result
	{
		get
		{
			if (_task != null)
			{
				return _task.GetAwaiter().GetResult();
			}
			return _result;
		}
	}

	public ValueTask(TResult result)
	{
		_task = null;
		_result = result;
	}

	public ValueTask(Task<TResult> task)
	{
		_task = task ?? throw new ArgumentNullException("task");
		_result = default(TResult);
	}

	public override int GetHashCode()
	{
		if (_task == null)
		{
			if (_result == null)
			{
				return 0;
			}
			return _result.GetHashCode();
		}
		return _task.GetHashCode();
	}

	public override bool Equals(object obj)
	{
		if (obj is ValueTask<TResult>)
		{
			return Equals((ValueTask<TResult>)obj);
		}
		return false;
	}

	public bool Equals(ValueTask<TResult> other)
	{
		if (_task == null && other._task == null)
		{
			return EqualityComparer<TResult>.Default.Equals(_result, other._result);
		}
		return _task == other._task;
	}

	public static bool operator ==(ValueTask<TResult> left, ValueTask<TResult> right)
	{
		return left.Equals(right);
	}

	public static bool operator !=(ValueTask<TResult> left, ValueTask<TResult> right)
	{
		return !left.Equals(right);
	}

	public Task<TResult> AsTask()
	{
		return _task ?? Task.FromResult(_result);
	}

	public ValueTaskAwaiter<TResult> GetAwaiter()
	{
		return new ValueTaskAwaiter<TResult>(this);
	}

	public ConfiguredValueTaskAwaitable<TResult> ConfigureAwait(bool continueOnCapturedContext)
	{
		return new ConfiguredValueTaskAwaitable<TResult>(this, continueOnCapturedContext);
	}

	public override string ToString()
	{
		if (_task != null)
		{
			if (_task.Status != TaskStatus.RanToCompletion || _task.Result == null)
			{
				return string.Empty;
			}
			return _task.Result.ToString();
		}
		if (_result == null)
		{
			return string.Empty;
		}
		return _result.ToString();
	}

	public static AsyncValueTaskMethodBuilder<TResult> CreateAsyncMethodBuilder()
	{
		return AsyncValueTaskMethodBuilder<TResult>.Create();
	}
}

using System.Runtime.InteropServices;
using System.Security;
using System.Threading.Tasks;

namespace System.Runtime.CompilerServices;

[StructLayout(LayoutKind.Auto)]
public struct AsyncValueTaskMethodBuilder<TResult>
{
	private AsyncTaskMethodBuilder<TResult> _methodBuilder;

	private TResult _result;

	private bool _haveResult;

	private bool _useBuilder;

	public ValueTask<TResult> Task
	{
		get
		{
			if (_haveResult)
			{
				return new ValueTask<TResult>(_result);
			}
			_useBuilder = true;
			return new ValueTask<TResult>(_methodBuilder.Task);
		}
	}

	public static AsyncValueTaskMethodBuilder<TResult> Create()
	{
		AsyncValueTaskMethodBuilder<TResult> result = default(AsyncValueTaskMethodBuilder<TResult>);
		result._methodBuilder = AsyncTaskMethodBuilder<TResult>.Create();
		return result;
	}

	public void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
	{
		_methodBuilder.Start(ref stateMachine);
	}

	public void SetStateMachine(IAsyncStateMachine stateMachine)
	{
		_methodBuilder.SetStateMachine(stateMachine);
	}

	public void SetResult(TResult result)
	{
		if (_useBuilder)
		{
			_methodBuilder.SetResult(result);
			return;
		}
		_result = result;
		_haveResult = true;
	}

	public void SetException(Exception exception)
	{
		_methodBuilder.SetException(exception);
	}

	public void AwaitOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine) where TAwaiter : INotifyCompletion where TStateMachine : IAsyncStateMachine
	{
		_useBuilder = true;
		_methodBuilder.AwaitOnCompleted(ref awaiter, ref stateMachine);
	}

	[SecuritySafeCritical]
	public void AwaitUnsafeOnCompleted<TAwaiter, TStateMachine>(ref TAwaiter awaiter, ref TStateMachine stateMachine) where TAwaiter : ICriticalNotifyCompletion where TStateMachine : IAsyncStateMachine
	{
		_useBuilder = true;
		_methodBuilder.AwaitUnsafeOnCompleted(ref awaiter, ref stateMachine);
	}
}

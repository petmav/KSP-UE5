using System.Security;

namespace System.Threading;

public sealed class AsyncLocal<T> : IAsyncLocal
{
	[SecurityCritical]
	private readonly Action<AsyncLocalValueChangedArgs<T>> m_valueChangedHandler;

	public T Value
	{
		[SecuritySafeCritical]
		get
		{
			object localValue = ExecutionContext.GetLocalValue(this);
			if (localValue != null)
			{
				return (T)localValue;
			}
			return default(T);
		}
		[SecuritySafeCritical]
		set
		{
			ExecutionContext.SetLocalValue(this, value, m_valueChangedHandler != null);
		}
	}

	public AsyncLocal()
	{
	}

	[SecurityCritical]
	public AsyncLocal(Action<AsyncLocalValueChangedArgs<T>> valueChangedHandler)
	{
		m_valueChangedHandler = valueChangedHandler;
	}

	[SecurityCritical]
	void IAsyncLocal.OnValueChanged(object previousValueObj, object currentValueObj, bool contextChanged)
	{
		T previousValue = ((previousValueObj == null) ? default(T) : ((T)previousValueObj));
		T currentValue = ((currentValueObj == null) ? default(T) : ((T)currentValueObj));
		m_valueChangedHandler(new AsyncLocalValueChangedArgs<T>(previousValue, currentValue, contextChanged));
	}
}

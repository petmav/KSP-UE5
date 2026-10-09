using System.Collections;

namespace System.Net;

internal sealed class InterlockedStack
{
	private readonly Stack _stack = new Stack();

	internal InterlockedStack()
	{
	}

	internal void Push(object pooledStream)
	{
		if (pooledStream == null)
		{
			throw new ArgumentNullException("pooledStream");
		}
		lock (_stack.SyncRoot)
		{
			_stack.Push(pooledStream);
		}
	}

	internal object Pop()
	{
		lock (_stack.SyncRoot)
		{
			object result = null;
			if (0 < _stack.Count)
			{
				result = _stack.Pop();
			}
			return result;
		}
	}
}

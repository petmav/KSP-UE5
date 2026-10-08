using System.Collections.Generic;
using Smooth.Delegates;
using Smooth.Dispose;

namespace Smooth.Pools
{
	public class KeyedPool<K, T>
	{
		private readonly Dictionary<K, Stack<T>> keyToValues = new Dictionary<K, Stack<T>>();

		private readonly DelegateFunc<K, T> create;

		private readonly DelegateFunc<T, K> reset;

		private readonly DelegateAction<T> release;

		private KeyedPool()
		{
		}

		public KeyedPool(DelegateFunc<K, T> create, DelegateFunc<T, K> reset)
		{
			this.create = create;
			this.reset = reset;
			release = Release;
		}

		public T Borrow(K key)
		{
			lock (keyToValues)
			{
				Stack<T> value;
				return (!keyToValues.TryGetValue(key, out value) || value.Count <= 0) ? create(key) : value.Pop();
			}
		}

		public void Release(T value)
		{
			K key = reset(value);
			lock (keyToValues)
			{
				if (!keyToValues.TryGetValue(key, out var value2))
				{
					value2 = new Stack<T>();
					keyToValues[key] = value2;
				}
				value2.Push(value);
			}
		}

		public Disposable<T> BorrowDisposable(K key)
		{
			return Disposable<T>.Borrow(Borrow(key), release);
		}
	}
}

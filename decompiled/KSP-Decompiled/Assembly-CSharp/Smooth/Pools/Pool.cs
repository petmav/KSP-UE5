using System;
using System.Collections.Generic;
using Smooth.Delegates;
using Smooth.Dispose;

namespace Smooth.Pools
{
	public class Pool<T>
	{
		private readonly Stack<T> values = new Stack<T>();

		private readonly DelegateFunc<T> create;

		private readonly DelegateAction<T> reset;

		private readonly DelegateAction<T> release;

		private PoolsStatus status = new PoolsStatus();

		public int Size => status.maxSize;

		public int Allocated => status.allocated;

		private Pool()
		{
		}

		public Pool(DelegateFunc<T> create, DelegateAction<T> reset)
		{
			this.create = create;
			this.reset = reset;
			release = Release;
			PoolsStatus.poolsInfo.Add(typeof(T), status);
		}

		public T Borrow()
		{
			lock (values)
			{
				if (values.Count > 0)
				{
					return values.Pop();
				}
				status.allocated++;
				return create();
			}
		}

		public void Release(T value)
		{
			reset(value);
			lock (values)
			{
				values.Push(value);
				status.maxSize = Math.Max(status.maxSize, values.Count);
			}
		}

		public Disposable<T> BorrowDisposable()
		{
			return Disposable<T>.Borrow(Borrow(), release);
		}
	}
}

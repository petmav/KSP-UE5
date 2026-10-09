using Smooth.Algebraics;
using Smooth.Delegates;
using Smooth.Dispose;

namespace Smooth.Pools
{
	public class KeyedPoolWithDefaultKey<K, T> : KeyedPool<K, T>
	{
		private readonly Either<K, DelegateFunc<K>> defaultKey;

		public KeyedPoolWithDefaultKey(DelegateFunc<K, T> create, DelegateFunc<T, K> reset, K defaultKey)
			: base(create, reset)
		{
			this.defaultKey = Either<K, DelegateFunc<K>>.Left(defaultKey);
		}

		public KeyedPoolWithDefaultKey(DelegateFunc<K, T> create, DelegateFunc<T, K> reset, DelegateFunc<K> defaultKeyFunc)
			: base(create, reset)
		{
			defaultKey = Either<K, DelegateFunc<K>>.Right(defaultKeyFunc);
		}

		public T Borrow()
		{
			return Borrow(defaultKey.isLeft ? defaultKey.leftValue : defaultKey.rightValue());
		}

		public Disposable<T> BorrowDisposable()
		{
			return BorrowDisposable(defaultKey.isLeft ? defaultKey.leftValue : defaultKey.rightValue());
		}
	}
}

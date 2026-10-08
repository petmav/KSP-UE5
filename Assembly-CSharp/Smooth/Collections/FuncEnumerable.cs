using System.Collections;
using System.Collections.Generic;
using Smooth.Algebraics;
using Smooth.Delegates;

namespace Smooth.Collections
{
	public class FuncEnumerable<T> : IEnumerable<T>, IEnumerable
	{
		private readonly T seed;

		private readonly Either<DelegateFunc<T, T>, DelegateFunc<T, Option<T>>> step;

		private FuncEnumerable()
		{
		}

		public FuncEnumerable(T seed, DelegateFunc<T, T> step)
		{
			this.seed = seed;
			this.step = Either<DelegateFunc<T, T>, DelegateFunc<T, Option<T>>>.Left(step);
		}

		public FuncEnumerable(T seed, DelegateFunc<T, Option<T>> step)
		{
			this.seed = seed;
			this.step = Either<DelegateFunc<T, T>, DelegateFunc<T, Option<T>>>.Right(step);
		}

		public IEnumerator<T> GetEnumerator()
		{
			if (step.isLeft)
			{
				T current = seed;
				while (true)
				{
					yield return current;
					current = step.leftValue(current);
				}
			}
			Option<T> current2 = new Option<T>(seed);
			while (current2.isSome)
			{
				yield return current2.value;
				current2 = step.rightValue(current2.value);
			}
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return GetEnumerator();
		}
	}
	public class FuncEnumerable<T, P> : IEnumerable<T>, IEnumerable
	{
		private readonly T seed;

		private readonly Either<DelegateFunc<T, P, T>, DelegateFunc<T, P, Option<T>>> step;

		private readonly P parameter;

		private FuncEnumerable()
		{
		}

		public FuncEnumerable(T seed, DelegateFunc<T, P, T> step, P parameter)
		{
			this.seed = seed;
			this.step = Either<DelegateFunc<T, P, T>, DelegateFunc<T, P, Option<T>>>.Left(step);
			this.parameter = parameter;
		}

		public FuncEnumerable(T seed, DelegateFunc<T, P, Option<T>> step, P parameter)
		{
			this.seed = seed;
			this.step = Either<DelegateFunc<T, P, T>, DelegateFunc<T, P, Option<T>>>.Right(step);
			this.parameter = parameter;
		}

		public IEnumerator<T> GetEnumerator()
		{
			if (step.isLeft)
			{
				T current = seed;
				while (true)
				{
					yield return current;
					current = step.leftValue(current, parameter);
				}
			}
			Option<T> current2 = new Option<T>(seed);
			while (current2.isSome)
			{
				yield return current2.value;
				current2 = step.rightValue(current2.value, parameter);
			}
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return GetEnumerator();
		}
	}
}

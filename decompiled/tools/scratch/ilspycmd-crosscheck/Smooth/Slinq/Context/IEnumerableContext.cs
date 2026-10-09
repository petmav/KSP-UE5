using System;
using System.Collections.Generic;
using Smooth.Algebraics;

namespace Smooth.Slinq.Context
{
	public struct IEnumerableContext<T>
	{
		private readonly IEnumerator<T> enumerator;

		private BacktrackDetector bd;

		private static readonly Mutator<T, IEnumerableContext<T>> skip = Skip;

		private static readonly Mutator<T, IEnumerableContext<T>> remove = Remove;

		private static readonly Mutator<T, IEnumerableContext<T>> dispose = Dispose;

		private IEnumerableContext(IEnumerable<T> enumerable)
		{
			enumerator = enumerable.GetEnumerator();
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<T, IEnumerableContext<T>> Slinq(IEnumerable<T> enumerable)
		{
			return new Slinq<T, IEnumerableContext<T>>(skip, remove, dispose, new IEnumerableContext<T>(enumerable));
		}

		private static void Skip(ref IEnumerableContext<T> context, out Option<T> next)
		{
			if (context.enumerator.MoveNext())
			{
				next = new Option<T>(context.enumerator.Current);
				return;
			}
			next = default(Option<T>);
			context.enumerator.Dispose();
		}

		private static void Remove(ref IEnumerableContext<T> context, out Option<T> next)
		{
			throw new NotSupportedException();
		}

		private static void Dispose(ref IEnumerableContext<T> context, out Option<T> next)
		{
			next = default(Option<T>);
			context.enumerator.Dispose();
		}
	}
}

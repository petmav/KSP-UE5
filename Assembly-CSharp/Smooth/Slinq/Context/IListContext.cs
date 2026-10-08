using System.Collections.Generic;
using Smooth.Algebraics;

namespace Smooth.Slinq.Context
{
	public struct IListContext<T>
	{
		private IList<T> list;

		private int size;

		private int index;

		private readonly int step;

		private BacktrackDetector bd;

		private static readonly Mutator<T, IListContext<T>> skip = Skip;

		private static readonly Mutator<T, IListContext<T>> remove = Remove;

		private static readonly Mutator<T, IListContext<T>> dispose = Dispose;

		private static readonly Mutator<Tuple<T, int>, IListContext<T>> skipWithIndex = SkipWithIndex;

		private static readonly Mutator<Tuple<T, int>, IListContext<T>> removeWithIndex = RemoveWithIndex;

		private static readonly Mutator<Tuple<T, int>, IListContext<T>> disposeWithIndex = DisposeWithIndex;

		private IListContext(IList<T> list, int startIndex, int step)
		{
			this.list = list;
			size = list.Count;
			index = startIndex - step;
			this.step = step;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<T, IListContext<T>> Slinq(IList<T> list, int startIndex, int step)
		{
			return new Slinq<T, IListContext<T>>(skip, remove, dispose, new IListContext<T>(list, startIndex, step));
		}

		public static Slinq<Tuple<T, int>, IListContext<T>> SlinqWithIndex(IList<T> list, int startIndex, int step)
		{
			return new Slinq<Tuple<T, int>, IListContext<T>>(skipWithIndex, removeWithIndex, disposeWithIndex, new IListContext<T>(list, startIndex, step));
		}

		private static void Skip(ref IListContext<T> context, out Option<T> next)
		{
			int num = context.index + context.step;
			if (0 <= num && num < context.size)
			{
				next = new Option<T>(context.list[num]);
				context.index = num;
			}
			else
			{
				next = default(Option<T>);
			}
		}

		private static void Remove(ref IListContext<T> context, out Option<T> next)
		{
			context.list.RemoveAt(context.index);
			if (context.step == 0)
			{
				next = default(Option<T>);
				return;
			}
			if (context.step > 0)
			{
				context.index--;
			}
			context.size--;
			Skip(ref context, out next);
		}

		private static void Dispose(ref IListContext<T> context, out Option<T> next)
		{
			next = default(Option<T>);
		}

		private static void SkipWithIndex(ref IListContext<T> context, out Option<Tuple<T, int>> next)
		{
			int num = context.index + context.step;
			if (0 <= num && num < context.size)
			{
				next = new Option<Tuple<T, int>>(new Tuple<T, int>(context.list[num], num));
				context.index = num;
			}
			else
			{
				next = default(Option<Tuple<T, int>>);
			}
		}

		private static void RemoveWithIndex(ref IListContext<T> context, out Option<Tuple<T, int>> next)
		{
			context.list.RemoveAt(context.index);
			if (context.step == 0)
			{
				next = default(Option<Tuple<T, int>>);
				return;
			}
			if (context.step > 0)
			{
				context.index--;
			}
			context.size--;
			SkipWithIndex(ref context, out next);
		}

		private static void DisposeWithIndex(ref IListContext<T> context, out Option<Tuple<T, int>> next)
		{
			next = default(Option<Tuple<T, int>>);
		}
	}
}

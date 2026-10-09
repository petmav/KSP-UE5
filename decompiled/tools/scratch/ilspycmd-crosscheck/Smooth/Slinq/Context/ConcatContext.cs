using Smooth.Algebraics;

namespace Smooth.Slinq.Context
{
	public struct ConcatContext<C2, T, C>
	{
		private bool needsMove;

		private Slinq<T, C> first;

		private Slinq<T, C2> second;

		private BacktrackDetector bd;

		private static readonly Mutator<T, ConcatContext<C2, T, C>> skip = Skip;

		private static readonly Mutator<T, ConcatContext<C2, T, C>> remove = Remove;

		private static readonly Mutator<T, ConcatContext<C2, T, C>> dispose = Dispose;

		private ConcatContext(Slinq<T, C> first, Slinq<T, C2> second)
		{
			needsMove = false;
			this.first = first;
			this.second = second;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<T, ConcatContext<C2, T, C>> Concat(Slinq<T, C> first, Slinq<T, C2> second)
		{
			return new Slinq<T, ConcatContext<C2, T, C>>(skip, remove, dispose, new ConcatContext<C2, T, C>(first, second));
		}

		private static void Skip(ref ConcatContext<C2, T, C> context, out Option<T> next)
		{
			if (context.needsMove)
			{
				if (context.first.current.isSome)
				{
					context.first.skip(ref context.first.context, out context.first.current);
				}
				else
				{
					context.second.skip(ref context.second.context, out context.second.current);
				}
			}
			else
			{
				context.needsMove = true;
			}
			next = (context.first.current.isSome ? context.first.current : context.second.current);
		}

		private static void Remove(ref ConcatContext<C2, T, C> context, out Option<T> next)
		{
			context.needsMove = false;
			if (context.first.current.isSome)
			{
				context.first.remove(ref context.first.context, out context.first.current);
			}
			else
			{
				context.second.remove(ref context.second.context, out context.second.current);
			}
			Skip(ref context, out next);
		}

		private static void Dispose(ref ConcatContext<C2, T, C> context, out Option<T> next)
		{
			next = default(Option<T>);
			if (context.first.current.isSome)
			{
				context.first.dispose(ref context.first.context, out context.first.current);
				if (context.second.current.isSome)
				{
					context.second.dispose(ref context.second.context, out context.second.current);
				}
			}
			else
			{
				context.second.dispose(ref context.second.context, out context.second.current);
			}
		}
	}
}

using Smooth.Algebraics;

namespace Smooth.Slinq.Context
{
	public struct IntContext<T, C>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private int count;

		private BacktrackDetector bd;

		private static readonly Mutator<T, IntContext<T, C>> skip = Skip;

		private static readonly Mutator<T, IntContext<T, C>> remove = Remove;

		private static readonly Mutator<T, IntContext<T, C>> dispose = Dispose;

		private IntContext(Slinq<T, C> chained, int count)
		{
			needsMove = false;
			this.chained = chained;
			this.count = count;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<T, IntContext<T, C>> Take(Slinq<T, C> slinq, int count)
		{
			return new Slinq<T, IntContext<T, C>>(skip, remove, dispose, new IntContext<T, C>(slinq, count));
		}

		private static void Skip(ref IntContext<T, C> context, out Option<T> next)
		{
			if (context.count-- > 0)
			{
				if (context.needsMove)
				{
					context.chained.skip(ref context.chained.context, out context.chained.current);
				}
				else
				{
					context.needsMove = true;
				}
			}
			else if (context.chained.current.isSome)
			{
				context.chained.dispose(ref context.chained.context, out context.chained.current);
			}
			next = context.chained.current;
		}

		private static void Remove(ref IntContext<T, C> context, out Option<T> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			Skip(ref context, out next);
		}

		private static void Dispose(ref IntContext<T, C> context, out Option<T> next)
		{
			next = default(Option<T>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}
	}
}

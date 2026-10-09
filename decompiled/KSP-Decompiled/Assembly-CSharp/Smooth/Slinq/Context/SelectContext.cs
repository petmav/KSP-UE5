using Smooth.Algebraics;
using Smooth.Delegates;

namespace Smooth.Slinq.Context
{
	public struct SelectContext<U, T, C>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private readonly DelegateFunc<T, U> selector;

		private BacktrackDetector bd;

		private static readonly Mutator<U, SelectContext<U, T, C>> skip = Skip;

		private static readonly Mutator<U, SelectContext<U, T, C>> remove = Remove;

		private static readonly Mutator<U, SelectContext<U, T, C>> dispose = Dispose;

		private SelectContext(Slinq<T, C> chained, DelegateFunc<T, U> selector)
		{
			needsMove = false;
			this.chained = chained;
			this.selector = selector;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, SelectContext<U, T, C>> Select(Slinq<T, C> slinq, DelegateFunc<T, U> selector)
		{
			return new Slinq<U, SelectContext<U, T, C>>(skip, remove, dispose, new SelectContext<U, T, C>(slinq, selector));
		}

		private static void Skip(ref SelectContext<U, T, C> context, out Option<U> next)
		{
			if (context.needsMove)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
			}
			else
			{
				context.needsMove = true;
			}
			next = (context.chained.current.isSome ? new Option<U>(context.selector(context.chained.current.value)) : default(Option<U>));
		}

		private static void Remove(ref SelectContext<U, T, C> context, out Option<U> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			Skip(ref context, out next);
		}

		private static void Dispose(ref SelectContext<U, T, C> context, out Option<U> next)
		{
			next = default(Option<U>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}
	}
	public struct SelectContext<U, T, C, P>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private readonly DelegateFunc<T, P, U> selector;

		private readonly P parameter;

		private BacktrackDetector bd;

		private static readonly Mutator<U, SelectContext<U, T, C, P>> skip = Skip;

		private static readonly Mutator<U, SelectContext<U, T, C, P>> remove = Remove;

		private static readonly Mutator<U, SelectContext<U, T, C, P>> dispose = Dispose;

		private SelectContext(Slinq<T, C> chained, DelegateFunc<T, P, U> selector, P parameter)
		{
			needsMove = false;
			this.chained = chained;
			this.selector = selector;
			this.parameter = parameter;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, SelectContext<U, T, C, P>> Select(Slinq<T, C> slinq, DelegateFunc<T, P, U> selector, P parameter)
		{
			return new Slinq<U, SelectContext<U, T, C, P>>(skip, remove, dispose, new SelectContext<U, T, C, P>(slinq, selector, parameter));
		}

		private static void Skip(ref SelectContext<U, T, C, P> context, out Option<U> next)
		{
			if (context.needsMove)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
			}
			else
			{
				context.needsMove = true;
			}
			next = (context.chained.current.isSome ? new Option<U>(context.selector(context.chained.current.value, context.parameter)) : default(Option<U>));
		}

		private static void Remove(ref SelectContext<U, T, C, P> context, out Option<U> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			Skip(ref context, out next);
		}

		private static void Dispose(ref SelectContext<U, T, C, P> context, out Option<U> next)
		{
			next = default(Option<U>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}
	}
}

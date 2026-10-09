using Smooth.Algebraics;
using Smooth.Delegates;

namespace Smooth.Slinq.Context
{
	public struct SelectSlinqContext<U, UC, T, C>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private Slinq<U, UC> selected;

		private readonly DelegateFunc<T, Slinq<U, UC>> selector;

		private BacktrackDetector bd;

		private static readonly Mutator<U, SelectSlinqContext<U, UC, T, C>> skip = Skip;

		private static readonly Mutator<U, SelectSlinqContext<U, UC, T, C>> remove = Remove;

		private static readonly Mutator<U, SelectSlinqContext<U, UC, T, C>> dispose = Dispose;

		private SelectSlinqContext(Slinq<T, C> chained, DelegateFunc<T, Slinq<U, UC>> selector)
		{
			needsMove = false;
			this.chained = chained;
			selected = (chained.current.isSome ? selector(chained.current.value) : default(Slinq<U, UC>));
			this.selector = selector;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, SelectSlinqContext<U, UC, T, C>> SelectMany(Slinq<T, C> slinq, DelegateFunc<T, Slinq<U, UC>> selector)
		{
			return new Slinq<U, SelectSlinqContext<U, UC, T, C>>(skip, remove, dispose, new SelectSlinqContext<U, UC, T, C>(slinq, selector));
		}

		private static void Skip(ref SelectSlinqContext<U, UC, T, C> context, out Option<U> next)
		{
			if (context.needsMove)
			{
				context.selected.skip(ref context.selected.context, out context.selected.current);
			}
			else
			{
				context.needsMove = true;
			}
			if (!context.selected.current.isSome && context.chained.current.isSome)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
				while (context.chained.current.isSome)
				{
					context.selected = context.selector(context.chained.current.value);
					if (context.selected.current.isSome)
					{
						break;
					}
					context.chained.skip(ref context.chained.context, out context.chained.current);
				}
			}
			next = context.selected.current;
		}

		private static void Remove(ref SelectSlinqContext<U, UC, T, C> context, out Option<U> next)
		{
			context.needsMove = false;
			context.selected.remove(ref context.selected.context, out context.selected.current);
			Skip(ref context, out next);
		}

		private static void Dispose(ref SelectSlinqContext<U, UC, T, C> context, out Option<U> next)
		{
			next = default(Option<U>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
			context.selected.dispose(ref context.selected.context, out context.selected.current);
		}
	}
	public struct SelectSlinqContext<U, UC, T, C, P>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private Slinq<U, UC> selected;

		private readonly DelegateFunc<T, P, Slinq<U, UC>> selector;

		private readonly P parameter;

		private BacktrackDetector bd;

		private static readonly Mutator<U, SelectSlinqContext<U, UC, T, C, P>> skip = Skip;

		private static readonly Mutator<U, SelectSlinqContext<U, UC, T, C, P>> remove = Remove;

		private static readonly Mutator<U, SelectSlinqContext<U, UC, T, C, P>> dispose = Dispose;

		private SelectSlinqContext(Slinq<T, C> chained, DelegateFunc<T, P, Slinq<U, UC>> selector, P parameter)
		{
			needsMove = false;
			this.chained = chained;
			selected = (chained.current.isSome ? selector(chained.current.value, parameter) : default(Slinq<U, UC>));
			this.selector = selector;
			this.parameter = parameter;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, SelectSlinqContext<U, UC, T, C, P>> SelectMany(Slinq<T, C> slinq, DelegateFunc<T, P, Slinq<U, UC>> selector, P parameter)
		{
			return new Slinq<U, SelectSlinqContext<U, UC, T, C, P>>(skip, remove, dispose, new SelectSlinqContext<U, UC, T, C, P>(slinq, selector, parameter));
		}

		private static void Skip(ref SelectSlinqContext<U, UC, T, C, P> context, out Option<U> next)
		{
			if (context.needsMove)
			{
				context.selected.skip(ref context.selected.context, out context.selected.current);
			}
			else
			{
				context.needsMove = true;
			}
			if (!context.selected.current.isSome && context.chained.current.isSome)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
				while (context.chained.current.isSome)
				{
					context.selected = context.selector(context.chained.current.value, context.parameter);
					if (context.selected.current.isSome)
					{
						break;
					}
					context.chained.skip(ref context.chained.context, out context.chained.current);
				}
			}
			next = context.selected.current;
		}

		private static void Remove(ref SelectSlinqContext<U, UC, T, C, P> context, out Option<U> next)
		{
			context.needsMove = false;
			context.selected.remove(ref context.selected.context, out context.selected.current);
			Skip(ref context, out next);
		}

		private static void Dispose(ref SelectSlinqContext<U, UC, T, C, P> context, out Option<U> next)
		{
			next = default(Option<U>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
			context.selected.dispose(ref context.selected.context, out context.selected.current);
		}
	}
}

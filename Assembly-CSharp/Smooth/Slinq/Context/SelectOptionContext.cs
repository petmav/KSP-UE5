using Smooth.Algebraics;
using Smooth.Delegates;

namespace Smooth.Slinq.Context
{
	public struct SelectOptionContext<U, T, C>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private readonly DelegateFunc<T, Option<U>> selector;

		private BacktrackDetector bd;

		private static readonly Mutator<U, SelectOptionContext<U, T, C>> skip = Skip;

		private static readonly Mutator<U, SelectOptionContext<U, T, C>> remove = Remove;

		private static readonly Mutator<U, SelectOptionContext<U, T, C>> dispose = Dispose;

		private SelectOptionContext(Slinq<T, C> chained, DelegateFunc<T, Option<U>> selector)
		{
			needsMove = false;
			this.chained = chained;
			this.selector = selector;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, SelectOptionContext<U, T, C>> SelectMany(Slinq<T, C> slinq, DelegateFunc<T, Option<U>> selector)
		{
			return new Slinq<U, SelectOptionContext<U, T, C>>(skip, remove, dispose, new SelectOptionContext<U, T, C>(slinq, selector));
		}

		private static void Skip(ref SelectOptionContext<U, T, C> context, out Option<U> next)
		{
			if (context.needsMove)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
			}
			else
			{
				context.needsMove = true;
			}
			if (context.chained.current.isSome)
			{
				Option<U> option = context.selector(context.chained.current.value);
				while (!option.isSome)
				{
					context.chained.skip(ref context.chained.context, out context.chained.current);
					if (!context.chained.current.isSome)
					{
						break;
					}
					option = context.selector(context.chained.current.value);
				}
				next = option;
			}
			else
			{
				next = default(Option<U>);
			}
		}

		private static void Remove(ref SelectOptionContext<U, T, C> context, out Option<U> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			Skip(ref context, out next);
		}

		private static void Dispose(ref SelectOptionContext<U, T, C> context, out Option<U> next)
		{
			next = default(Option<U>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}
	}
	public struct SelectOptionContext<U, T, C, P>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private readonly DelegateFunc<T, P, Option<U>> selector;

		private readonly P parameter;

		private BacktrackDetector bd;

		private static readonly Mutator<U, SelectOptionContext<U, T, C, P>> skip = Skip;

		private static readonly Mutator<U, SelectOptionContext<U, T, C, P>> remove = Remove;

		private static readonly Mutator<U, SelectOptionContext<U, T, C, P>> dispose = Dispose;

		private SelectOptionContext(Slinq<T, C> chained, DelegateFunc<T, P, Option<U>> selector, P parameter)
		{
			needsMove = false;
			this.chained = chained;
			this.selector = selector;
			this.parameter = parameter;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, SelectOptionContext<U, T, C, P>> SelectMany(Slinq<T, C> slinq, DelegateFunc<T, P, Option<U>> selector, P parameter)
		{
			return new Slinq<U, SelectOptionContext<U, T, C, P>>(skip, remove, dispose, new SelectOptionContext<U, T, C, P>(slinq, selector, parameter));
		}

		private static void Skip(ref SelectOptionContext<U, T, C, P> context, out Option<U> next)
		{
			if (context.needsMove)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
			}
			else
			{
				context.needsMove = true;
			}
			if (context.chained.current.isSome)
			{
				Option<U> option = context.selector(context.chained.current.value, context.parameter);
				while (!option.isSome)
				{
					context.chained.skip(ref context.chained.context, out context.chained.current);
					if (!context.chained.current.isSome)
					{
						break;
					}
					option = context.selector(context.chained.current.value, context.parameter);
				}
				next = option;
			}
			else
			{
				next = default(Option<U>);
			}
		}

		private static void Remove(ref SelectOptionContext<U, T, C, P> context, out Option<U> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			Skip(ref context, out next);
		}

		private static void Dispose(ref SelectOptionContext<U, T, C, P> context, out Option<U> next)
		{
			next = default(Option<U>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}
	}
}

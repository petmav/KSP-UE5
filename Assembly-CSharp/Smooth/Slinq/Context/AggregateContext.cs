using Smooth.Algebraics;
using Smooth.Delegates;

namespace Smooth.Slinq.Context
{
	public struct AggregateContext<U, T, C>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private U acc;

		private readonly DelegateFunc<U, T, U> selector;

		private BacktrackDetector bd;

		private static readonly Mutator<U, AggregateContext<U, T, C>> skip = Skip;

		private static readonly Mutator<U, AggregateContext<U, T, C>> remove = Remove;

		private static readonly Mutator<U, AggregateContext<U, T, C>> dispose = Dispose;

		private AggregateContext(Slinq<T, C> chained, U seed, DelegateFunc<U, T, U> selector)
		{
			needsMove = false;
			this.chained = chained;
			acc = seed;
			this.selector = selector;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, AggregateContext<U, T, C>> AggregateRunning(Slinq<T, C> slinq, U seed, DelegateFunc<U, T, U> selector)
		{
			return new Slinq<U, AggregateContext<U, T, C>>(skip, remove, dispose, new AggregateContext<U, T, C>(slinq, seed, selector));
		}

		private static void Skip(ref AggregateContext<U, T, C> context, out Option<U> next)
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
				context.acc = context.selector(context.acc, context.chained.current.value);
				next = new Option<U>(context.acc);
			}
			else
			{
				next = default(Option<U>);
			}
		}

		private static void Remove(ref AggregateContext<U, T, C> context, out Option<U> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			Skip(ref context, out next);
		}

		private static void Dispose(ref AggregateContext<U, T, C> context, out Option<U> next)
		{
			next = default(Option<U>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}
	}
	public struct AggregateContext<U, T, C, P>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private U acc;

		private readonly DelegateFunc<U, T, P, U> selector;

		private readonly P parameter;

		private BacktrackDetector bd;

		private static readonly Mutator<U, AggregateContext<U, T, C, P>> skip = Skip;

		private static readonly Mutator<U, AggregateContext<U, T, C, P>> remove = Remove;

		private static readonly Mutator<U, AggregateContext<U, T, C, P>> dispose = Dispose;

		private AggregateContext(Slinq<T, C> chained, U seed, DelegateFunc<U, T, P, U> selector, P parameter)
		{
			needsMove = false;
			this.chained = chained;
			acc = seed;
			this.selector = selector;
			this.parameter = parameter;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, AggregateContext<U, T, C, P>> AggregateRunning(Slinq<T, C> slinq, U seed, DelegateFunc<U, T, P, U> selector, P parameter)
		{
			return new Slinq<U, AggregateContext<U, T, C, P>>(skip, remove, dispose, new AggregateContext<U, T, C, P>(slinq, seed, selector, parameter));
		}

		private static void Skip(ref AggregateContext<U, T, C, P> context, out Option<U> next)
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
				context.acc = context.selector(context.acc, context.chained.current.value, context.parameter);
				next = new Option<U>(context.acc);
			}
			else
			{
				next = default(Option<U>);
			}
		}

		private static void Remove(ref AggregateContext<U, T, C, P> context, out Option<U> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			Skip(ref context, out next);
		}

		private static void Dispose(ref AggregateContext<U, T, C, P> context, out Option<U> next)
		{
			next = default(Option<U>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}
	}
}

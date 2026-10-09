using Smooth.Algebraics;
using Smooth.Delegates;

namespace Smooth.Slinq.Context
{
	public struct PredicateContext<T, C>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private readonly DelegateFunc<T, bool> predicate;

		private BacktrackDetector bd;

		private static readonly Mutator<T, PredicateContext<T, C>> dispose = Dispose;

		private static readonly Mutator<T, PredicateContext<T, C>> takeWhileSkip = TakeWhileSkip;

		private static readonly Mutator<T, PredicateContext<T, C>> takeWhileRemove = TakeWhileRemove;

		private static readonly Mutator<T, PredicateContext<T, C>> whereSkip = WhereSkip;

		private static readonly Mutator<T, PredicateContext<T, C>> whereRemove = WhereRemove;

		private PredicateContext(Slinq<T, C> chained, DelegateFunc<T, bool> predicate)
		{
			needsMove = false;
			this.chained = chained;
			this.predicate = predicate;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<T, PredicateContext<T, C>> TakeWhile(Slinq<T, C> slinq, DelegateFunc<T, bool> predicate)
		{
			return new Slinq<T, PredicateContext<T, C>>(takeWhileSkip, takeWhileRemove, dispose, new PredicateContext<T, C>(slinq, predicate));
		}

		public static Slinq<T, PredicateContext<T, C>> Where(Slinq<T, C> slinq, DelegateFunc<T, bool> predicate)
		{
			return new Slinq<T, PredicateContext<T, C>>(whereSkip, whereRemove, dispose, new PredicateContext<T, C>(slinq, predicate));
		}

		private static void Dispose(ref PredicateContext<T, C> context, out Option<T> next)
		{
			next = default(Option<T>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}

		private static void TakeWhileSkip(ref PredicateContext<T, C> context, out Option<T> next)
		{
			if (context.needsMove)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
			}
			else
			{
				context.needsMove = true;
			}
			if (!context.chained.current.isSome)
			{
				next = context.chained.current;
				return;
			}
			if (context.predicate(context.chained.current.value))
			{
				next = context.chained.current;
				return;
			}
			next = default(Option<T>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}

		private static void TakeWhileRemove(ref PredicateContext<T, C> context, out Option<T> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			TakeWhileSkip(ref context, out next);
		}

		private static void WhereSkip(ref PredicateContext<T, C> context, out Option<T> next)
		{
			if (context.needsMove)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
			}
			else
			{
				context.needsMove = true;
			}
			while (context.chained.current.isSome && !context.predicate(context.chained.current.value))
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
			}
			next = context.chained.current;
		}

		private static void WhereRemove(ref PredicateContext<T, C> context, out Option<T> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			WhereSkip(ref context, out next);
		}
	}
	public struct PredicateContext<T, C, P>
	{
		private bool needsMove;

		private Slinq<T, C> chained;

		private readonly DelegateFunc<T, P, bool> predicate;

		private readonly P parameter;

		private BacktrackDetector bd;

		private static readonly Mutator<T, PredicateContext<T, C, P>> dispose = Dispose;

		private static readonly Mutator<T, PredicateContext<T, C, P>> takeWhileSkip = TakeWhileSkip;

		private static readonly Mutator<T, PredicateContext<T, C, P>> takeWhileRemove = TakeWhileRemove;

		private static readonly Mutator<T, PredicateContext<T, C, P>> whereSkip = WhereSkip;

		private static readonly Mutator<T, PredicateContext<T, C, P>> whereRemove = WhereRemove;

		private PredicateContext(Slinq<T, C> chained, DelegateFunc<T, P, bool> predicate, P parameter)
		{
			needsMove = false;
			this.chained = chained;
			this.predicate = predicate;
			this.parameter = parameter;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<T, PredicateContext<T, C, P>> TakeWhile(Slinq<T, C> slinq, DelegateFunc<T, P, bool> predicate, P parameter)
		{
			return new Slinq<T, PredicateContext<T, C, P>>(takeWhileSkip, takeWhileRemove, dispose, new PredicateContext<T, C, P>(slinq, predicate, parameter));
		}

		public static Slinq<T, PredicateContext<T, C, P>> Where(Slinq<T, C> slinq, DelegateFunc<T, P, bool> predicate, P parameter)
		{
			return new Slinq<T, PredicateContext<T, C, P>>(whereSkip, whereRemove, dispose, new PredicateContext<T, C, P>(slinq, predicate, parameter));
		}

		private static void Dispose(ref PredicateContext<T, C, P> context, out Option<T> next)
		{
			next = default(Option<T>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}

		private static void TakeWhileSkip(ref PredicateContext<T, C, P> context, out Option<T> next)
		{
			if (context.needsMove)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
			}
			else
			{
				context.needsMove = true;
			}
			if (!context.chained.current.isSome)
			{
				next = context.chained.current;
				return;
			}
			if (context.predicate(context.chained.current.value, context.parameter))
			{
				next = context.chained.current;
				return;
			}
			next = default(Option<T>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
		}

		private static void TakeWhileRemove(ref PredicateContext<T, C, P> context, out Option<T> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			TakeWhileSkip(ref context, out next);
		}

		private static void WhereSkip(ref PredicateContext<T, C, P> context, out Option<T> next)
		{
			if (context.needsMove)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
			}
			else
			{
				context.needsMove = true;
			}
			while (context.chained.current.isSome && !context.predicate(context.chained.current.value, context.parameter))
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
			}
			next = context.chained.current;
		}

		private static void WhereRemove(ref PredicateContext<T, C, P> context, out Option<T> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			WhereSkip(ref context, out next);
		}
	}
}

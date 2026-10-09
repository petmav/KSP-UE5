using Smooth.Algebraics;
using Smooth.Delegates;
using Smooth.Slinq.Collections;

namespace Smooth.Slinq.Context
{
	public struct JoinContext<U, K, T2, T, C>
	{
		private bool needsMove;

		private readonly Lookup<K, T2> lookup;

		private readonly DelegateFunc<T, K> outerSelector;

		private readonly DelegateFunc<T, T2, U> resultSelector;

		private readonly bool release;

		private Slinq<T, C> chained;

		private Linked<T2> inner;

		private BacktrackDetector bd;

		private static readonly Mutator<U, JoinContext<U, K, T2, T, C>> skip = Skip;

		private static readonly Mutator<U, JoinContext<U, K, T2, T, C>> remove = Remove;

		private static readonly Mutator<U, JoinContext<U, K, T2, T, C>> dispose = Dispose;

		private JoinContext(Lookup<K, T2> lookup, Slinq<T, C> outer, DelegateFunc<T, K> outerSelector, DelegateFunc<T, T2, U> resultSelector, bool release)
		{
			needsMove = false;
			this.lookup = lookup;
			this.outerSelector = outerSelector;
			this.resultSelector = resultSelector;
			chained = outer;
			this.release = release;
			inner = ((!chained.current.isSome || !lookup.dictionary.TryGetValue(outerSelector(chained.current.value), out var value)) ? null : value.head);
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, JoinContext<U, K, T2, T, C>> Join(Lookup<K, T2> lookup, Slinq<T, C> outer, DelegateFunc<T, K> outerSelector, DelegateFunc<T, T2, U> resultSelector, bool release)
		{
			return new Slinq<U, JoinContext<U, K, T2, T, C>>(skip, remove, dispose, new JoinContext<U, K, T2, T, C>(lookup, outer, outerSelector, resultSelector, release));
		}

		private static void Skip(ref JoinContext<U, K, T2, T, C> context, out Option<U> next)
		{
			if (context.needsMove)
			{
				context.inner = context.inner.next;
			}
			else
			{
				context.needsMove = true;
			}
			if (context.inner == null && context.chained.current.isSome)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
				while (context.chained.current.isSome)
				{
					context.inner = context.lookup.GetValues(context.outerSelector(context.chained.current.value)).head;
					if (context.inner != null)
					{
						break;
					}
					context.chained.skip(ref context.chained.context, out context.chained.current);
				}
			}
			if (context.chained.current.isSome)
			{
				next = new Option<U>(context.resultSelector(context.chained.current.value, context.inner.value));
				return;
			}
			next = default(Option<U>);
			if (context.release)
			{
				context.lookup.DisposeInBackground();
			}
		}

		private static void Remove(ref JoinContext<U, K, T2, T, C> context, out Option<U> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			context.inner = (context.chained.current.isSome ? context.lookup.GetValues(context.outerSelector(context.chained.current.value)).head : null);
			Skip(ref context, out next);
		}

		private static void Dispose(ref JoinContext<U, K, T2, T, C> context, out Option<U> next)
		{
			next = default(Option<U>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
			if (context.release)
			{
				context.lookup.DisposeInBackground();
			}
		}
	}
	public struct JoinContext<U, K, T2, T, C, P>
	{
		private bool needsMove;

		private readonly Lookup<K, T2> lookup;

		private readonly DelegateFunc<T, P, K> outerSelector;

		private readonly DelegateFunc<T, T2, P, U> resultSelector;

		private readonly P parameter;

		private readonly bool release;

		private Slinq<T, C> chained;

		private Linked<T2> inner;

		private BacktrackDetector bd;

		private static readonly Mutator<U, JoinContext<U, K, T2, T, C, P>> skip = Skip;

		private static readonly Mutator<U, JoinContext<U, K, T2, T, C, P>> remove = Remove;

		private static readonly Mutator<U, JoinContext<U, K, T2, T, C, P>> dispose = Dispose;

		private JoinContext(Lookup<K, T2> lookup, Slinq<T, C> outer, DelegateFunc<T, P, K> outerSelector, DelegateFunc<T, T2, P, U> resultSelector, P parameter, bool release)
		{
			needsMove = false;
			this.lookup = lookup;
			this.outerSelector = outerSelector;
			this.resultSelector = resultSelector;
			this.parameter = parameter;
			chained = outer;
			this.release = release;
			inner = ((!chained.current.isSome || !lookup.dictionary.TryGetValue(outerSelector(chained.current.value, parameter), out var value)) ? null : value.head);
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, JoinContext<U, K, T2, T, C, P>> Join(Lookup<K, T2> lookup, Slinq<T, C> outer, DelegateFunc<T, P, K> outerSelector, DelegateFunc<T, T2, P, U> resultSelector, P parameter, bool release)
		{
			return new Slinq<U, JoinContext<U, K, T2, T, C, P>>(skip, remove, dispose, new JoinContext<U, K, T2, T, C, P>(lookup, outer, outerSelector, resultSelector, parameter, release));
		}

		private static void Skip(ref JoinContext<U, K, T2, T, C, P> context, out Option<U> next)
		{
			if (context.needsMove)
			{
				context.inner = context.inner.next;
			}
			else
			{
				context.needsMove = true;
			}
			if (context.inner == null && context.chained.current.isSome)
			{
				context.chained.skip(ref context.chained.context, out context.chained.current);
				while (context.chained.current.isSome)
				{
					context.inner = context.lookup.GetValues(context.outerSelector(context.chained.current.value, context.parameter)).head;
					if (context.inner != null)
					{
						break;
					}
					context.chained.skip(ref context.chained.context, out context.chained.current);
				}
			}
			if (context.chained.current.isSome)
			{
				next = new Option<U>(context.resultSelector(context.chained.current.value, context.inner.value, context.parameter));
				return;
			}
			next = default(Option<U>);
			if (context.release)
			{
				context.lookup.DisposeInBackground();
			}
		}

		private static void Remove(ref JoinContext<U, K, T2, T, C, P> context, out Option<U> next)
		{
			context.needsMove = false;
			context.chained.remove(ref context.chained.context, out context.chained.current);
			context.inner = (context.chained.current.isSome ? context.lookup.GetValues(context.outerSelector(context.chained.current.value, context.parameter)).head : null);
			Skip(ref context, out next);
		}

		private static void Dispose(ref JoinContext<U, K, T2, T, C, P> context, out Option<U> next)
		{
			next = default(Option<U>);
			context.chained.dispose(ref context.chained.context, out context.chained.current);
			if (context.release)
			{
				context.lookup.DisposeInBackground();
			}
		}
	}
}

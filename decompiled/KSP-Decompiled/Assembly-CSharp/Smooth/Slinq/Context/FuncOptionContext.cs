using System;
using Smooth.Algebraics;
using Smooth.Delegates;

namespace Smooth.Slinq.Context
{
	public struct FuncOptionContext<T>
	{
		private bool needsMove;

		private Option<T> acc;

		private readonly DelegateFunc<T, Option<T>> selector;

		private BacktrackDetector bd;

		private static readonly Mutator<T, FuncOptionContext<T>> skip = Skip;

		private static readonly Mutator<T, FuncOptionContext<T>> remove = skip;

		private static readonly Mutator<T, FuncOptionContext<T>> dispose = Dispose;

		private FuncOptionContext(T seed, DelegateFunc<T, Option<T>> selector)
		{
			needsMove = false;
			acc = new Option<T>(seed);
			this.selector = selector;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<T, FuncOptionContext<T>> Sequence(T seed, DelegateFunc<T, Option<T>> selector)
		{
			return new Slinq<T, FuncOptionContext<T>>(skip, remove, dispose, new FuncOptionContext<T>(seed, selector));
		}

		private static void Skip(ref FuncOptionContext<T> context, out Option<T> next)
		{
			if (context.needsMove)
			{
				context.acc = context.selector(context.acc.value);
			}
			else
			{
				context.needsMove = true;
			}
			next = context.acc;
		}

		private static void Remove(ref FuncOptionContext<T> context, out Option<T> next)
		{
			throw new NotSupportedException();
		}

		private static void Dispose(ref FuncOptionContext<T> context, out Option<T> next)
		{
			next = default(Option<T>);
		}
	}
	public struct FuncOptionContext<T, P>
	{
		private bool needsMove;

		private Option<T> acc;

		private readonly DelegateFunc<T, P, Option<T>> selector;

		private readonly P parameter;

		private BacktrackDetector bd;

		private static readonly Mutator<T, FuncOptionContext<T, P>> skip = Skip;

		private static readonly Mutator<T, FuncOptionContext<T, P>> remove = skip;

		private static readonly Mutator<T, FuncOptionContext<T, P>> dispose = Dispose;

		private FuncOptionContext(T seed, DelegateFunc<T, P, Option<T>> selector, P parameter)
		{
			needsMove = false;
			acc = new Option<T>(seed);
			this.selector = selector;
			this.parameter = parameter;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<T, FuncOptionContext<T, P>> Sequence(T seed, DelegateFunc<T, P, Option<T>> selector, P parameter)
		{
			return new Slinq<T, FuncOptionContext<T, P>>(skip, remove, dispose, new FuncOptionContext<T, P>(seed, selector, parameter));
		}

		private static void Skip(ref FuncOptionContext<T, P> context, out Option<T> next)
		{
			if (context.needsMove)
			{
				context.acc = context.selector(context.acc.value, context.parameter);
			}
			else
			{
				context.needsMove = true;
			}
			next = context.acc;
		}

		private static void Remove(ref FuncOptionContext<T, P> context, out Option<T> next)
		{
			throw new NotSupportedException();
		}

		private static void Dispose(ref FuncOptionContext<T, P> context, out Option<T> next)
		{
			next = default(Option<T>);
		}
	}
}

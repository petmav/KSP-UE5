using Smooth.Algebraics;
using Smooth.Delegates;

namespace Smooth.Slinq.Context
{
	public struct ZipAllContext<T2, C2, T, C>
	{
		private bool needsMove;

		private Slinq<T, C> left;

		private Slinq<T2, C2> right;

		private readonly ZipRemoveFlags removeFlags;

		private BacktrackDetector bd;

		private static readonly Mutator<Tuple<Option<T>, Option<T2>>, ZipAllContext<T2, C2, T, C>> skip = Skip;

		private static readonly Mutator<Tuple<Option<T>, Option<T2>>, ZipAllContext<T2, C2, T, C>> remove = Remove;

		private static readonly Mutator<Tuple<Option<T>, Option<T2>>, ZipAllContext<T2, C2, T, C>> dispose = Dispose;

		private ZipAllContext(Slinq<T, C> left, Slinq<T2, C2> right, ZipRemoveFlags removeFlags)
		{
			needsMove = false;
			this.left = left;
			this.right = right;
			this.removeFlags = removeFlags;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<Tuple<Option<T>, Option<T2>>, ZipAllContext<T2, C2, T, C>> ZipAll(Slinq<T, C> left, Slinq<T2, C2> right, ZipRemoveFlags removeFlags)
		{
			return new Slinq<Tuple<Option<T>, Option<T2>>, ZipAllContext<T2, C2, T, C>>(skip, remove, dispose, new ZipAllContext<T2, C2, T, C>(left, right, removeFlags));
		}

		private static void Skip(ref ZipAllContext<T2, C2, T, C> context, out Option<Tuple<Option<T>, Option<T2>>> next)
		{
			if (context.needsMove)
			{
				if (context.left.current.isSome)
				{
					context.left.skip(ref context.left.context, out context.left.current);
					if (context.right.current.isSome)
					{
						context.right.skip(ref context.right.context, out context.right.current);
					}
				}
				else
				{
					context.right.skip(ref context.right.context, out context.right.current);
				}
			}
			else
			{
				context.needsMove = true;
			}
			if (!context.left.current.isSome && !context.right.current.isSome)
			{
				next = default(Option<Tuple<Option<T>, Option<T2>>>);
			}
			else
			{
				next = new Option<Tuple<Option<T>, Option<T2>>>(new Tuple<Option<T>, Option<T2>>(context.left.current, context.right.current));
			}
		}

		private static void Remove(ref ZipAllContext<T2, C2, T, C> context, out Option<Tuple<Option<T>, Option<T2>>> next)
		{
			context.needsMove = false;
			if (context.left.current.isSome)
			{
				if ((context.removeFlags & ZipRemoveFlags.Left) == ZipRemoveFlags.Left)
				{
					context.left.remove(ref context.left.context, out context.left.current);
				}
				else
				{
					context.left.skip(ref context.left.context, out context.left.current);
				}
			}
			if (context.right.current.isSome)
			{
				if ((context.removeFlags & ZipRemoveFlags.Right) == ZipRemoveFlags.Right)
				{
					context.right.remove(ref context.right.context, out context.right.current);
				}
				else
				{
					context.right.skip(ref context.right.context, out context.right.current);
				}
			}
			Skip(ref context, out next);
		}

		private static void Dispose(ref ZipAllContext<T2, C2, T, C> context, out Option<Tuple<Option<T>, Option<T2>>> next)
		{
			next = default(Option<Tuple<Option<T>, Option<T2>>>);
			if (context.left.current.isSome)
			{
				context.left.dispose(ref context.left.context, out context.left.current);
				if (context.right.current.isSome)
				{
					context.right.dispose(ref context.right.context, out context.right.current);
				}
			}
			else
			{
				context.right.dispose(ref context.right.context, out context.right.current);
			}
		}
	}
	public struct ZipAllContext<U, T2, C2, T, C>
	{
		private bool needsMove;

		private Slinq<T, C> left;

		private Slinq<T2, C2> right;

		private readonly DelegateFunc<Option<T>, Option<T2>, U> selector;

		private readonly ZipRemoveFlags removeFlags;

		private BacktrackDetector bd;

		private static readonly Mutator<U, ZipAllContext<U, T2, C2, T, C>> skip = Skip;

		private static readonly Mutator<U, ZipAllContext<U, T2, C2, T, C>> remove = Remove;

		private static readonly Mutator<U, ZipAllContext<U, T2, C2, T, C>> dispose = Dispose;

		private ZipAllContext(Slinq<T, C> left, Slinq<T2, C2> right, DelegateFunc<Option<T>, Option<T2>, U> selector, ZipRemoveFlags removeFlags)
		{
			needsMove = false;
			this.left = left;
			this.right = right;
			this.selector = selector;
			this.removeFlags = removeFlags;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, ZipAllContext<U, T2, C2, T, C>> ZipAll(Slinq<T, C> left, Slinq<T2, C2> right, DelegateFunc<Option<T>, Option<T2>, U> selector, ZipRemoveFlags removeFlags)
		{
			return new Slinq<U, ZipAllContext<U, T2, C2, T, C>>(skip, remove, dispose, new ZipAllContext<U, T2, C2, T, C>(left, right, selector, removeFlags));
		}

		private static void Skip(ref ZipAllContext<U, T2, C2, T, C> context, out Option<U> next)
		{
			if (context.needsMove)
			{
				if (context.left.current.isSome)
				{
					context.left.skip(ref context.left.context, out context.left.current);
					if (context.right.current.isSome)
					{
						context.right.skip(ref context.right.context, out context.right.current);
					}
				}
				else
				{
					context.right.skip(ref context.right.context, out context.right.current);
				}
			}
			else
			{
				context.needsMove = true;
			}
			if (!context.left.current.isSome && !context.right.current.isSome)
			{
				next = default(Option<U>);
			}
			else
			{
				next = new Option<U>(context.selector(context.left.current, context.right.current));
			}
		}

		private static void Remove(ref ZipAllContext<U, T2, C2, T, C> context, out Option<U> next)
		{
			context.needsMove = false;
			if (context.left.current.isSome)
			{
				if ((context.removeFlags & ZipRemoveFlags.Left) == ZipRemoveFlags.Left)
				{
					context.left.remove(ref context.left.context, out context.left.current);
				}
				else
				{
					context.left.skip(ref context.left.context, out context.left.current);
				}
			}
			if (context.right.current.isSome)
			{
				if ((context.removeFlags & ZipRemoveFlags.Right) == ZipRemoveFlags.Right)
				{
					context.right.remove(ref context.right.context, out context.right.current);
				}
				else
				{
					context.right.skip(ref context.right.context, out context.right.current);
				}
			}
			Skip(ref context, out next);
		}

		private static void Dispose(ref ZipAllContext<U, T2, C2, T, C> context, out Option<U> next)
		{
			next = default(Option<U>);
			if (context.left.current.isSome)
			{
				context.left.dispose(ref context.left.context, out context.left.current);
				if (context.right.current.isSome)
				{
					context.right.dispose(ref context.right.context, out context.right.current);
				}
			}
			else
			{
				context.right.dispose(ref context.right.context, out context.right.current);
			}
		}
	}
	public struct ZipAllContext<U, T2, C2, T, C, P>
	{
		private bool needsMove;

		private Slinq<T, C> left;

		private Slinq<T2, C2> right;

		private readonly DelegateFunc<Option<T>, Option<T2>, P, U> selector;

		private readonly P parameter;

		private readonly ZipRemoveFlags removeFlags;

		private BacktrackDetector bd;

		private static readonly Mutator<U, ZipAllContext<U, T2, C2, T, C, P>> skip = Skip;

		private static readonly Mutator<U, ZipAllContext<U, T2, C2, T, C, P>> remove = Remove;

		private static readonly Mutator<U, ZipAllContext<U, T2, C2, T, C, P>> dispose = Dispose;

		private ZipAllContext(Slinq<T, C> left, Slinq<T2, C2> right, DelegateFunc<Option<T>, Option<T2>, P, U> selector, P parameter, ZipRemoveFlags removeFlags)
		{
			needsMove = false;
			this.left = left;
			this.right = right;
			this.selector = selector;
			this.parameter = parameter;
			this.removeFlags = removeFlags;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<U, ZipAllContext<U, T2, C2, T, C, P>> ZipAll(Slinq<T, C> left, Slinq<T2, C2> right, DelegateFunc<Option<T>, Option<T2>, P, U> selector, P parameter, ZipRemoveFlags removeFlags)
		{
			return new Slinq<U, ZipAllContext<U, T2, C2, T, C, P>>(skip, remove, dispose, new ZipAllContext<U, T2, C2, T, C, P>(left, right, selector, parameter, removeFlags));
		}

		private static void Skip(ref ZipAllContext<U, T2, C2, T, C, P> context, out Option<U> next)
		{
			if (context.needsMove)
			{
				if (context.left.current.isSome)
				{
					context.left.skip(ref context.left.context, out context.left.current);
					if (context.right.current.isSome)
					{
						context.right.skip(ref context.right.context, out context.right.current);
					}
				}
				else
				{
					context.right.skip(ref context.right.context, out context.right.current);
				}
			}
			else
			{
				context.needsMove = true;
			}
			if (!context.left.current.isSome && !context.right.current.isSome)
			{
				next = default(Option<U>);
			}
			else
			{
				next = new Option<U>(context.selector(context.left.current, context.right.current, context.parameter));
			}
		}

		private static void Remove(ref ZipAllContext<U, T2, C2, T, C, P> context, out Option<U> next)
		{
			context.needsMove = false;
			if (context.left.current.isSome)
			{
				if ((context.removeFlags & ZipRemoveFlags.Left) == ZipRemoveFlags.Left)
				{
					context.left.remove(ref context.left.context, out context.left.current);
				}
				else
				{
					context.left.skip(ref context.left.context, out context.left.current);
				}
			}
			if (context.right.current.isSome)
			{
				if ((context.removeFlags & ZipRemoveFlags.Right) == ZipRemoveFlags.Right)
				{
					context.right.remove(ref context.right.context, out context.right.current);
				}
				else
				{
					context.right.skip(ref context.right.context, out context.right.current);
				}
			}
			Skip(ref context, out next);
		}

		private static void Dispose(ref ZipAllContext<U, T2, C2, T, C, P> context, out Option<U> next)
		{
			next = default(Option<U>);
			if (context.left.current.isSome)
			{
				context.left.dispose(ref context.left.context, out context.left.current);
				if (context.right.current.isSome)
				{
					context.right.dispose(ref context.right.context, out context.right.current);
				}
			}
			else
			{
				context.right.dispose(ref context.right.context, out context.right.current);
			}
		}
	}
}

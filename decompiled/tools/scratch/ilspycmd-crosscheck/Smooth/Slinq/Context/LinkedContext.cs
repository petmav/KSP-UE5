using System;
using Smooth.Algebraics;
using Smooth.Slinq.Collections;

namespace Smooth.Slinq.Context
{
	public struct LinkedContext<T>
	{
		private readonly LinkedHeadTail<T> list;

		private readonly bool release;

		private Linked<T> runner;

		private BacktrackDetector bd;

		private static readonly Mutator<T, LinkedContext<T>> skip = Skip;

		private static readonly Mutator<T, LinkedContext<T>> remove = Remove;

		private static readonly Mutator<T, LinkedContext<T>> dispose = Dispose;

		private LinkedContext(LinkedHeadTail<T> list, BacktrackDetector bd, bool release)
		{
			this.list = list;
			runner = list.head;
			this.release = release;
			this.bd = bd;
		}

		public static Slinq<T, LinkedContext<T>> Slinq(LinkedHeadTail<T> list, bool release)
		{
			return new Slinq<T, LinkedContext<T>>(skip, remove, dispose, new LinkedContext<T>(list, BacktrackDetector.Borrow(), release));
		}

		public static Slinq<T, LinkedContext<T>> Slinq(LinkedHeadTail<T> list, BacktrackDetector bd, bool release)
		{
			return new Slinq<T, LinkedContext<T>>(skip, remove, dispose, new LinkedContext<T>(list, bd, release));
		}

		private static void Skip(ref LinkedContext<T> context, out Option<T> next)
		{
			if (context.runner == null)
			{
				next = default(Option<T>);
				if (context.release)
				{
					context.list.DisposeInBackground();
				}
			}
			else
			{
				next = new Option<T>(context.runner.value);
				context.runner = context.runner.next;
			}
		}

		private static void Remove(ref LinkedContext<T> context, out Option<T> next)
		{
			throw new NotSupportedException();
		}

		private static void Dispose(ref LinkedContext<T> context, out Option<T> next)
		{
			next = default(Option<T>);
			if (context.release)
			{
				context.list.DisposeInBackground();
			}
		}
	}
	public struct LinkedContext<K, T>
	{
		private readonly LinkedHeadTail<K, T> list;

		private readonly bool release;

		private Linked<K, T> runner;

		private BacktrackDetector bd;

		private static readonly Mutator<T, LinkedContext<K, T>> skip = Skip;

		private static readonly Mutator<T, LinkedContext<K, T>> remove = Remove;

		private static readonly Mutator<T, LinkedContext<K, T>> dispose = Dispose;

		private LinkedContext(LinkedHeadTail<K, T> list, BacktrackDetector bd, bool release)
		{
			this.list = list;
			runner = list.head;
			this.release = release;
			this.bd = bd;
		}

		public static Slinq<T, LinkedContext<K, T>> Slinq(LinkedHeadTail<K, T> list, bool release)
		{
			return new Slinq<T, LinkedContext<K, T>>(skip, remove, dispose, new LinkedContext<K, T>(list, BacktrackDetector.Borrow(), release));
		}

		public static Slinq<T, LinkedContext<K, T>> Slinq(LinkedHeadTail<K, T> list, BacktrackDetector bd, bool release)
		{
			return new Slinq<T, LinkedContext<K, T>>(skip, remove, dispose, new LinkedContext<K, T>(list, bd, release));
		}

		private static void Skip(ref LinkedContext<K, T> context, out Option<T> next)
		{
			if (context.runner == null)
			{
				next = default(Option<T>);
				if (context.release)
				{
					context.list.DisposeInBackground();
				}
			}
			else
			{
				next = new Option<T>(context.runner.value);
				context.runner = context.runner.next;
			}
		}

		private static void Remove(ref LinkedContext<K, T> context, out Option<T> next)
		{
			throw new NotSupportedException();
		}

		private static void Dispose(ref LinkedContext<K, T> context, out Option<T> next)
		{
			next = default(Option<T>);
			if (context.release)
			{
				context.list.DisposeInBackground();
			}
		}
	}
}

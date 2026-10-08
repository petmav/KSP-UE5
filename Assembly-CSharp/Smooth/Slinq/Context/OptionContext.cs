using System;
using Smooth.Algebraics;

namespace Smooth.Slinq.Context
{
	public struct OptionContext<T>
	{
		private Option<T> option;

		private BacktrackDetector bd;

		private static readonly Mutator<T, OptionContext<T>> remove = Remove;

		private static readonly Mutator<T, OptionContext<T>> dispose = Dispose;

		private static readonly Mutator<T, OptionContext<T>> optionSkip = OptionSkip;

		private static readonly Mutator<T, OptionContext<T>> repeatSkip = RepeatSkip;

		private OptionContext(Option<T> option)
		{
			this.option = option;
			bd = BacktrackDetector.Borrow();
		}

		public static Slinq<T, OptionContext<T>> Slinq(Option<T> option)
		{
			return new Slinq<T, OptionContext<T>>(optionSkip, remove, dispose, new OptionContext<T>(option));
		}

		public static Slinq<T, OptionContext<T>> Repeat(T value)
		{
			return new Slinq<T, OptionContext<T>>(repeatSkip, remove, dispose, new OptionContext<T>(new Option<T>(value)));
		}

		private static void Remove(ref OptionContext<T> context, out Option<T> next)
		{
			throw new NotSupportedException();
		}

		private static void Dispose(ref OptionContext<T> context, out Option<T> next)
		{
			next = default(Option<T>);
		}

		private static void OptionSkip(ref OptionContext<T> context, out Option<T> next)
		{
			next = context.option;
			if (context.option.isSome)
			{
				context.option = default(Option<T>);
			}
		}

		private static void RepeatSkip(ref OptionContext<T> context, out Option<T> next)
		{
			next = context.option;
		}
	}
}

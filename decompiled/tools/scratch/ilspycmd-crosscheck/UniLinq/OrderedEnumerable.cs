using System;
using System.Collections;
using System.Collections.Generic;

namespace UniLinq
{
	internal abstract class OrderedEnumerable<TElement> : IOrderedEnumerable<TElement>, IEnumerable<TElement>, IEnumerable
	{
		private IEnumerable<TElement> source;

		protected OrderedEnumerable(IEnumerable<TElement> source)
		{
			this.source = source;
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return GetEnumerator();
		}

		public virtual IEnumerator<TElement> GetEnumerator()
		{
			return Sort(source).GetEnumerator();
		}

		public abstract SortContext<TElement> CreateContext(SortContext<TElement> current);

		protected abstract IEnumerable<TElement> Sort(IEnumerable<TElement> source);

		public IOrderedEnumerable<TElement> CreateOrderedEnumerable<TKey>(Func<TElement, TKey> selector, IComparer<TKey> comparer, bool descending)
		{
			return new OrderedSequence<TElement, TKey>(this, source, selector, comparer, descending ? SortDirection.Descending : SortDirection.Ascending);
		}
	}
}

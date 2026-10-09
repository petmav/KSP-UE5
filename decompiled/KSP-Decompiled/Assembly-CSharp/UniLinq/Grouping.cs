using System.Collections;
using System.Collections.Generic;

namespace UniLinq
{
	internal class Grouping<K, T> : IGrouping<K, T>, IEnumerable<T>, IEnumerable
	{
		private K key;

		private IEnumerable<T> group;

		public K Key
		{
			get
			{
				return key;
			}
			set
			{
				key = value;
			}
		}

		public Grouping(K key, IEnumerable<T> group)
		{
			this.group = group;
			this.key = key;
		}

		public IEnumerator<T> GetEnumerator()
		{
			return group.GetEnumerator();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return group.GetEnumerator();
		}
	}
}

using System.Collections.Generic;

namespace Smooth.Collections
{
	public static class SortedList
	{
		public static SortedList<K, V> Create<K, V>()
		{
			return new SortedList<K, V>(Comparer<K>.Default);
		}

		public static SortedList<K, V> Create<K, V>(int capacity)
		{
			return new SortedList<K, V>(capacity, Comparer<K>.Default);
		}

		public static SortedList<K, V> Create<K, V>(IDictionary<K, V> dictionary)
		{
			return new SortedList<K, V>(dictionary, Comparer<K>.Default);
		}
	}
}

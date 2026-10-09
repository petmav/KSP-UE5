using System.Collections.Generic;

namespace Smooth.Collections
{
	public static class SortedDictionary
	{
		public static SortedDictionary<K, V> Create<K, V>()
		{
			return new SortedDictionary<K, V>(Comparer<K>.Default);
		}

		public static SortedDictionary<K, V> Create<K, V>(IDictionary<K, V> dictionary)
		{
			return new SortedDictionary<K, V>(dictionary, Comparer<K>.Default);
		}
	}
}

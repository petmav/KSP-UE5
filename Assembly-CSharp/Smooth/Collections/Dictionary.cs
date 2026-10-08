using System.Collections.Generic;

namespace Smooth.Collections
{
	public static class Dictionary
	{
		public static Dictionary<K, V> Create<K, V>()
		{
			return new Dictionary<K, V>(EqualityComparer<K>.Default);
		}

		public static Dictionary<K, V> Create<K, V>(int capacity)
		{
			return new Dictionary<K, V>(capacity, EqualityComparer<K>.Default);
		}

		public static Dictionary<K, V> Create<K, V>(IDictionary<K, V> dictionary)
		{
			return new Dictionary<K, V>(dictionary, EqualityComparer<K>.Default);
		}
	}
}

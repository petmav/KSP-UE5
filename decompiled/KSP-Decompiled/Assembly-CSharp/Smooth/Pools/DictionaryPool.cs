using System.Collections.Generic;
using Smooth.Collections;

namespace Smooth.Pools
{
	public static class DictionaryPool<K, V>
	{
		private static readonly KeyedPoolWithDefaultKey<IEqualityComparer<K>, Dictionary<K, V>> _Instance = new KeyedPoolWithDefaultKey<IEqualityComparer<K>, Dictionary<K, V>>((IEqualityComparer<K> comparer) => new Dictionary<K, V>(comparer), delegate(Dictionary<K, V> dictionary)
		{
			dictionary.Clear();
			return dictionary.Comparer;
		}, () => Smooth.Collections.EqualityComparer<K>.Default);

		public static KeyedPoolWithDefaultKey<IEqualityComparer<K>, Dictionary<K, V>> Instance => _Instance;
	}
}

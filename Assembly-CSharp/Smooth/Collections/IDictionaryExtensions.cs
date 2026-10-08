using System.Collections.Generic;
using Smooth.Algebraics;

namespace Smooth.Collections
{
	public static class IDictionaryExtensions
	{
		public static Option<V> TryGet<K, V>(this IDictionary<K, V> dictionary, K key)
		{
			if (!dictionary.TryGetValue(key, out var value))
			{
				return default(Option<V>);
			}
			return new Option<V>(value);
		}
	}
}

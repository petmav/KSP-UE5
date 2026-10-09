using System.Collections.Generic;
using Smooth.Collections;

namespace Smooth.Compare.Comparers
{
	public class KeyValuePairEqualityComparer<K, V> : Smooth.Collections.EqualityComparer<KeyValuePair<K, V>>
	{
		public override bool Equals(KeyValuePair<K, V> l, KeyValuePair<K, V> r)
		{
			if (Smooth.Collections.EqualityComparer<K>.Default.Equals(l.Key, r.Key))
			{
				return Smooth.Collections.EqualityComparer<V>.Default.Equals(l.Value, r.Value);
			}
			return false;
		}

		public override int GetHashCode(KeyValuePair<K, V> kvp)
		{
			int num = 17;
			num = 493 + Smooth.Collections.EqualityComparer<K>.Default.GetHashCode(kvp.Key);
			return 29 * num + Smooth.Collections.EqualityComparer<V>.Default.GetHashCode(kvp.Value);
		}
	}
}

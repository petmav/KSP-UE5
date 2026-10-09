using System.Collections.Generic;
using Smooth.Collections;

namespace Smooth.Compare.Comparers
{
	public class KeyValuePairComparer<K, V> : Smooth.Collections.Comparer<KeyValuePair<K, V>>
	{
		public override int Compare(KeyValuePair<K, V> l, KeyValuePair<K, V> r)
		{
			int num = Smooth.Collections.Comparer<K>.Default.Compare(l.Key, r.Key);
			if (num != 0)
			{
				return num;
			}
			return Smooth.Collections.Comparer<V>.Default.Compare(l.Value, r.Value);
		}
	}
}

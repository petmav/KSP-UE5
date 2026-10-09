using System.Collections.Generic;

namespace Smooth.Collections
{
	public static class ICollectionExtensions
	{
		public static IC AddAll<IC, T>(this IC collection, params T[] values) where IC : ICollection<T>
		{
			for (int i = 0; i < values.Length; i++)
			{
				collection.Add(values[i]);
			}
			return collection;
		}

		public static IC AddAll<IC, T>(this IC collection, IList<T> values) where IC : ICollection<T>
		{
			for (int i = 0; i < values.Count; i++)
			{
				collection.Add(values[i]);
			}
			return collection;
		}

		public static IC AddAll<IC, T>(this IC collection, IEnumerable<T> values) where IC : ICollection<T>
		{
			foreach (T value in values)
			{
				collection.Add(value);
			}
			return collection;
		}
	}
}

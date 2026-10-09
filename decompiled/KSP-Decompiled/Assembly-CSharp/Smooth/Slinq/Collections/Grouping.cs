namespace Smooth.Slinq.Collections
{
	public struct Grouping<K, T>
	{
		public readonly K key;

		public LinkedHeadTail<T> values;

		public Grouping(K key, LinkedHeadTail<T> values)
		{
			this.key = key;
			this.values = values;
		}
	}
	public struct Grouping<K, T, C>
	{
		public readonly K key;

		public Slinq<T, C> values;

		public Grouping(K key, Slinq<T, C> values)
		{
			this.key = key;
			this.values = values;
		}
	}
	public static class Grouping
	{
		public static Grouping<K, T> Create<K, T>(K key, LinkedHeadTail<T> values)
		{
			return new Grouping<K, T>(key, values);
		}

		public static Grouping<K, T, C> Create<K, T, C>(K key, Slinq<T, C> values)
		{
			return new Grouping<K, T, C>(key, values);
		}
	}
}

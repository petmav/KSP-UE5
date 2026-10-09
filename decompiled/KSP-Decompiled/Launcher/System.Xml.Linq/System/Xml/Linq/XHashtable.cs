using System.Threading;

namespace System.Xml.Linq;

internal sealed class XHashtable<TValue>
{
	public delegate string ExtractKeyDelegate(TValue value);

	private sealed class XHashtableState
	{
		private struct Entry
		{
			public TValue Value;

			public int HashCode;

			public int Next;
		}

		private int[] buckets;

		private Entry[] entries;

		private int numEntries;

		private ExtractKeyDelegate extractKey;

		private const int EndOfList = 0;

		private const int FullList = -1;

		public XHashtableState(ExtractKeyDelegate extractKey, int capacity)
		{
			buckets = new int[capacity];
			entries = new Entry[capacity];
			this.extractKey = extractKey;
		}

		public XHashtableState Resize()
		{
			if (numEntries < buckets.Length)
			{
				return this;
			}
			int num = 0;
			for (int i = 0; i < buckets.Length; i++)
			{
				int num2 = buckets[i];
				if (num2 == 0)
				{
					num2 = Interlocked.CompareExchange(ref buckets[i], -1, 0);
				}
				while (num2 > 0)
				{
					if (extractKey(entries[num2].Value) != null)
					{
						num++;
					}
					num2 = ((entries[num2].Next != 0) ? entries[num2].Next : Interlocked.CompareExchange(ref entries[num2].Next, -1, 0));
				}
			}
			if (num < buckets.Length / 2)
			{
				num = buckets.Length;
			}
			else
			{
				num = buckets.Length * 2;
				if (num < 0)
				{
					throw new OverflowException();
				}
			}
			XHashtableState xHashtableState = new XHashtableState(extractKey, num);
			for (int j = 0; j < buckets.Length; j++)
			{
				for (int num3 = buckets[j]; num3 > 0; num3 = entries[num3].Next)
				{
					xHashtableState.TryAdd(entries[num3].Value, out var _);
				}
			}
			return xHashtableState;
		}

		public bool TryGetValue(string key, int index, int count, out TValue value)
		{
			int hashCode = ComputeHashCode(key, index, count);
			int entryIndex = 0;
			if (FindEntry(hashCode, key, index, count, ref entryIndex))
			{
				value = entries[entryIndex].Value;
				return true;
			}
			value = default(TValue);
			return false;
		}

		public bool TryAdd(TValue value, out TValue newValue)
		{
			newValue = value;
			string text = extractKey(value);
			if (text == null)
			{
				return true;
			}
			int num = ComputeHashCode(text, 0, text.Length);
			int num2 = Interlocked.Increment(ref numEntries);
			if (num2 < 0 || num2 >= buckets.Length)
			{
				return false;
			}
			entries[num2].Value = value;
			entries[num2].HashCode = num;
			Thread.MemoryBarrier();
			int entryIndex = 0;
			while (!FindEntry(num, text, 0, text.Length, ref entryIndex))
			{
				entryIndex = ((entryIndex != 0) ? Interlocked.CompareExchange(ref entries[entryIndex].Next, num2, 0) : Interlocked.CompareExchange(ref buckets[num & (buckets.Length - 1)], num2, 0));
				if (entryIndex <= 0)
				{
					return entryIndex == 0;
				}
			}
			newValue = entries[entryIndex].Value;
			return true;
		}

		private bool FindEntry(int hashCode, string key, int index, int count, ref int entryIndex)
		{
			int num = entryIndex;
			int num2 = ((num != 0) ? num : buckets[hashCode & (buckets.Length - 1)]);
			while (num2 > 0)
			{
				if (entries[num2].HashCode == hashCode)
				{
					string text = extractKey(entries[num2].Value);
					if (text == null)
					{
						if (entries[num2].Next > 0)
						{
							entries[num2].Value = default(TValue);
							num2 = entries[num2].Next;
							if (num == 0)
							{
								buckets[hashCode & (buckets.Length - 1)] = num2;
							}
							else
							{
								entries[num].Next = num2;
							}
							continue;
						}
					}
					else if (count == text.Length && string.CompareOrdinal(key, index, text, 0, count) == 0)
					{
						entryIndex = num2;
						return true;
					}
				}
				num = num2;
				num2 = entries[num2].Next;
			}
			entryIndex = num;
			return false;
		}

		private static int ComputeHashCode(string key, int index, int count)
		{
			int num = 352654597;
			int num2 = index + count;
			for (int i = index; i < num2; i++)
			{
				num += (num << 7) ^ key[i];
			}
			num -= num >> 17;
			num -= num >> 11;
			num -= num >> 5;
			return num & 0x7FFFFFFF;
		}
	}

	private XHashtableState state;

	private const int StartingHash = 352654597;

	public XHashtable(ExtractKeyDelegate extractKey, int capacity)
	{
		state = new XHashtableState(extractKey, capacity);
	}

	public bool TryGetValue(string key, int index, int count, out TValue value)
	{
		return state.TryGetValue(key, index, count, out value);
	}

	public TValue Add(TValue value)
	{
		TValue newValue;
		while (!state.TryAdd(value, out newValue))
		{
			lock (this)
			{
				XHashtableState xHashtableState = state.Resize();
				Thread.MemoryBarrier();
				state = xHashtableState;
			}
		}
		return newValue;
	}
}

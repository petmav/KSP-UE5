using System.Diagnostics;
using System.Runtime.Serialization;
using System.Security;
using System.Threading;

namespace System.Collections.Generic;

[Serializable]
[DebuggerTypeProxy(typeof(IDictionaryDebugView<, >))]
[DebuggerDisplay("Count = {Count}")]
public class Dictionary<TKey, TValue> : IDictionary<TKey, TValue>, ICollection<KeyValuePair<TKey, TValue>>, IEnumerable<KeyValuePair<TKey, TValue>>, IEnumerable, IDictionary, ICollection, IReadOnlyDictionary<TKey, TValue>, IReadOnlyCollection<KeyValuePair<TKey, TValue>>, ISerializable, IDeserializationCallback
{
	private struct Entry
	{
		public int hashCode;

		public int next;

		public TKey key;

		public TValue value;
	}

	[Serializable]
	public struct Enumerator : IEnumerator<KeyValuePair<TKey, TValue>>, IDisposable, IEnumerator, IDictionaryEnumerator
	{
		private Dictionary<TKey, TValue> dictionary;

		private int version;

		private int index;

		private KeyValuePair<TKey, TValue> current;

		private int getEnumeratorRetType;

		internal const int DictEntry = 1;

		internal const int KeyValuePair = 2;

		public KeyValuePair<TKey, TValue> Current => current;

		object IEnumerator.Current
		{
			get
			{
				if (index == 0 || index == dictionary.count + 1)
				{
					throw new InvalidOperationException("Enumeration has either not started or has already finished.");
				}
				if (getEnumeratorRetType == 1)
				{
					return new DictionaryEntry(current.Key, current.Value);
				}
				return new KeyValuePair<TKey, TValue>(current.Key, current.Value);
			}
		}

		DictionaryEntry IDictionaryEnumerator.Entry
		{
			get
			{
				if (index == 0 || index == dictionary.count + 1)
				{
					throw new InvalidOperationException("Enumeration has either not started or has already finished.");
				}
				return new DictionaryEntry(current.Key, current.Value);
			}
		}

		object IDictionaryEnumerator.Key
		{
			get
			{
				if (index == 0 || index == dictionary.count + 1)
				{
					throw new InvalidOperationException("Enumeration has either not started or has already finished.");
				}
				return current.Key;
			}
		}

		object IDictionaryEnumerator.Value
		{
			get
			{
				if (index == 0 || index == dictionary.count + 1)
				{
					throw new InvalidOperationException("Enumeration has either not started or has already finished.");
				}
				return current.Value;
			}
		}

		internal Enumerator(Dictionary<TKey, TValue> dictionary, int getEnumeratorRetType)
		{
			this.dictionary = dictionary;
			version = dictionary.version;
			index = 0;
			this.getEnumeratorRetType = getEnumeratorRetType;
			current = default(KeyValuePair<TKey, TValue>);
		}

		public bool MoveNext()
		{
			if (version != dictionary.version)
			{
				throw new InvalidOperationException("Collection was modified; enumeration operation may not execute.");
			}
			while ((uint)index < (uint)dictionary.count)
			{
				if (dictionary.entries[index].hashCode >= 0)
				{
					current = new KeyValuePair<TKey, TValue>(dictionary.entries[index].key, dictionary.entries[index].value);
					index++;
					return true;
				}
				index++;
			}
			index = dictionary.count + 1;
			current = default(KeyValuePair<TKey, TValue>);
			return false;
		}

		public void Dispose()
		{
		}

		void IEnumerator.Reset()
		{
			if (version != dictionary.version)
			{
				throw new InvalidOperationException("Collection was modified; enumeration operation may not execute.");
			}
			index = 0;
			current = default(KeyValuePair<TKey, TValue>);
		}
	}

	[Serializable]
	[DebuggerTypeProxy(typeof(DictionaryKeyCollectionDebugView<, >))]
	[DebuggerDisplay("Count = {Count}")]
	public sealed class KeyCollection : ICollection<TKey>, IEnumerable<TKey>, IEnumerable, ICollection, IReadOnlyCollection<TKey>
	{
		[Serializable]
		public struct Enumerator : IEnumerator<TKey>, IDisposable, IEnumerator
		{
			private Dictionary<TKey, TValue> dictionary;

			private int index;

			private int version;

			private TKey currentKey;

			public TKey Current => currentKey;

			object IEnumerator.Current
			{
				get
				{
					if (index == 0 || index == dictionary.count + 1)
					{
						throw new InvalidOperationException("Enumeration has either not started or has already finished.");
					}
					return currentKey;
				}
			}

			internal Enumerator(Dictionary<TKey, TValue> dictionary)
			{
				this.dictionary = dictionary;
				version = dictionary.version;
				index = 0;
				currentKey = default(TKey);
			}

			public void Dispose()
			{
			}

			public bool MoveNext()
			{
				if (version != dictionary.version)
				{
					throw new InvalidOperationException("Collection was modified; enumeration operation may not execute.");
				}
				while ((uint)index < (uint)dictionary.count)
				{
					if (dictionary.entries[index].hashCode >= 0)
					{
						currentKey = dictionary.entries[index].key;
						index++;
						return true;
					}
					index++;
				}
				index = dictionary.count + 1;
				currentKey = default(TKey);
				return false;
			}

			void IEnumerator.Reset()
			{
				if (version != dictionary.version)
				{
					throw new InvalidOperationException("Collection was modified; enumeration operation may not execute.");
				}
				index = 0;
				currentKey = default(TKey);
			}
		}

		private Dictionary<TKey, TValue> dictionary;

		public int Count => dictionary.Count;

		bool ICollection<TKey>.IsReadOnly => true;

		bool ICollection.IsSynchronized => false;

		object ICollection.SyncRoot => ((ICollection)dictionary).SyncRoot;

		public KeyCollection(Dictionary<TKey, TValue> dictionary)
		{
			if (dictionary == null)
			{
				throw new ArgumentNullException("dictionary");
			}
			this.dictionary = dictionary;
		}

		public Enumerator GetEnumerator()
		{
			return new Enumerator(dictionary);
		}

		public void CopyTo(TKey[] array, int index)
		{
			if (array == null)
			{
				throw new ArgumentNullException("array");
			}
			if (index < 0 || index > array.Length)
			{
				throw new ArgumentOutOfRangeException("index", index, "Index was out of range. Must be non-negative and less than the size of the collection.");
			}
			if (array.Length - index < dictionary.Count)
			{
				throw new ArgumentException("Destination array is not long enough to copy all the items in the collection. Check array index and length.");
			}
			int count = dictionary.count;
			Entry[] entries = dictionary.entries;
			for (int i = 0; i < count; i++)
			{
				if (entries[i].hashCode >= 0)
				{
					array[index++] = entries[i].key;
				}
			}
		}

		void ICollection<TKey>.Add(TKey item)
		{
			throw new NotSupportedException("Mutating a key collection derived from a dictionary is not allowed.");
		}

		void ICollection<TKey>.Clear()
		{
			throw new NotSupportedException("Mutating a key collection derived from a dictionary is not allowed.");
		}

		bool ICollection<TKey>.Contains(TKey item)
		{
			return dictionary.ContainsKey(item);
		}

		bool ICollection<TKey>.Remove(TKey item)
		{
			throw new NotSupportedException("Mutating a key collection derived from a dictionary is not allowed.");
		}

		IEnumerator<TKey> IEnumerable<TKey>.GetEnumerator()
		{
			return new Enumerator(dictionary);
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return new Enumerator(dictionary);
		}

		void ICollection.CopyTo(Array array, int index)
		{
			if (array == null)
			{
				throw new ArgumentNullException("array");
			}
			if (array.Rank != 1)
			{
				throw new ArgumentException("Only single dimensional arrays are supported for the requested action.", "array");
			}
			if (array.GetLowerBound(0) != 0)
			{
				throw new ArgumentException("The lower bound of target array must be zero.", "array");
			}
			if (index < 0 || index > array.Length)
			{
				throw new ArgumentOutOfRangeException("index", index, "Index was out of range. Must be non-negative and less than the size of the collection.");
			}
			if (array.Length - index < dictionary.Count)
			{
				throw new ArgumentException("Destination array is not long enough to copy all the items in the collection. Check array index and length.");
			}
			if (array is TKey[] array2)
			{
				CopyTo(array2, index);
				return;
			}
			if (!(array is object[] array3))
			{
				throw new ArgumentException("Target array type is not compatible with the type of items in the collection.", "array");
			}
			int count = dictionary.count;
			Entry[] entries = dictionary.entries;
			try
			{
				for (int i = 0; i < count; i++)
				{
					if (entries[i].hashCode >= 0)
					{
						array3[index++] = entries[i].key;
					}
				}
			}
			catch (ArrayTypeMismatchException)
			{
				throw new ArgumentException("Target array type is not compatible with the type of items in the collection.", "array");
			}
		}
	}

	[Serializable]
	[DebuggerDisplay("Count = {Count}")]
	[DebuggerTypeProxy(typeof(DictionaryValueCollectionDebugView<, >))]
	public sealed class ValueCollection : ICollection<TValue>, IEnumerable<TValue>, IEnumerable, ICollection, IReadOnlyCollection<TValue>
	{
		[Serializable]
		public struct Enumerator : IEnumerator<TValue>, IDisposable, IEnumerator
		{
			private Dictionary<TKey, TValue> dictionary;

			private int index;

			private int version;

			private TValue currentValue;

			public TValue Current => currentValue;

			object IEnumerator.Current
			{
				get
				{
					if (index == 0 || index == dictionary.count + 1)
					{
						throw new InvalidOperationException("Enumeration has either not started or has already finished.");
					}
					return currentValue;
				}
			}

			internal Enumerator(Dictionary<TKey, TValue> dictionary)
			{
				this.dictionary = dictionary;
				version = dictionary.version;
				index = 0;
				currentValue = default(TValue);
			}

			public void Dispose()
			{
			}

			public bool MoveNext()
			{
				if (version != dictionary.version)
				{
					throw new InvalidOperationException("Collection was modified; enumeration operation may not execute.");
				}
				while ((uint)index < (uint)dictionary.count)
				{
					if (dictionary.entries[index].hashCode >= 0)
					{
						currentValue = dictionary.entries[index].value;
						index++;
						return true;
					}
					index++;
				}
				index = dictionary.count + 1;
				currentValue = default(TValue);
				return false;
			}

			void IEnumerator.Reset()
			{
				if (version != dictionary.version)
				{
					throw new InvalidOperationException("Collection was modified; enumeration operation may not execute.");
				}
				index = 0;
				currentValue = default(TValue);
			}
		}

		private Dictionary<TKey, TValue> dictionary;

		public int Count => dictionary.Count;

		bool ICollection<TValue>.IsReadOnly => true;

		bool ICollection.IsSynchronized => false;

		object ICollection.SyncRoot => ((ICollection)dictionary).SyncRoot;

		public ValueCollection(Dictionary<TKey, TValue> dictionary)
		{
			if (dictionary == null)
			{
				throw new ArgumentNullException("dictionary");
			}
			this.dictionary = dictionary;
		}

		public Enumerator GetEnumerator()
		{
			return new Enumerator(dictionary);
		}

		public void CopyTo(TValue[] array, int index)
		{
			if (array == null)
			{
				throw new ArgumentNullException("array");
			}
			if (index < 0 || index > array.Length)
			{
				throw new ArgumentOutOfRangeException("index", index, "Index was out of range. Must be non-negative and less than the size of the collection.");
			}
			if (array.Length - index < dictionary.Count)
			{
				throw new ArgumentException("Destination array is not long enough to copy all the items in the collection. Check array index and length.");
			}
			int count = dictionary.count;
			Entry[] entries = dictionary.entries;
			for (int i = 0; i < count; i++)
			{
				if (entries[i].hashCode >= 0)
				{
					array[index++] = entries[i].value;
				}
			}
		}

		void ICollection<TValue>.Add(TValue item)
		{
			throw new NotSupportedException("Mutating a value collection derived from a dictionary is not allowed.");
		}

		bool ICollection<TValue>.Remove(TValue item)
		{
			throw new NotSupportedException("Mutating a value collection derived from a dictionary is not allowed.");
		}

		void ICollection<TValue>.Clear()
		{
			throw new NotSupportedException("Mutating a value collection derived from a dictionary is not allowed.");
		}

		bool ICollection<TValue>.Contains(TValue item)
		{
			return dictionary.ContainsValue(item);
		}

		IEnumerator<TValue> IEnumerable<TValue>.GetEnumerator()
		{
			return new Enumerator(dictionary);
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return new Enumerator(dictionary);
		}

		void ICollection.CopyTo(Array array, int index)
		{
			if (array == null)
			{
				throw new ArgumentNullException("array");
			}
			if (array.Rank != 1)
			{
				throw new ArgumentException("Only single dimensional arrays are supported for the requested action.", "array");
			}
			if (array.GetLowerBound(0) != 0)
			{
				throw new ArgumentException("The lower bound of target array must be zero.", "array");
			}
			if (index < 0 || index > array.Length)
			{
				throw new ArgumentOutOfRangeException("index", index, "Index was out of range. Must be non-negative and less than the size of the collection.");
			}
			if (array.Length - index < dictionary.Count)
			{
				throw new ArgumentException("Destination array is not long enough to copy all the items in the collection. Check array index and length.");
			}
			if (array is TValue[] array2)
			{
				CopyTo(array2, index);
				return;
			}
			if (!(array is object[] array3))
			{
				throw new ArgumentException("Target array type is not compatible with the type of items in the collection.", "array");
			}
			int count = dictionary.count;
			Entry[] entries = dictionary.entries;
			try
			{
				for (int i = 0; i < count; i++)
				{
					if (entries[i].hashCode >= 0)
					{
						array3[index++] = entries[i].value;
					}
				}
			}
			catch (ArrayTypeMismatchException)
			{
				throw new ArgumentException("Target array type is not compatible with the type of items in the collection.", "array");
			}
		}
	}

	private int[] buckets;

	private Entry[] entries;

	private int count;

	private int version;

	private int freeList;

	private int freeCount;

	private IEqualityComparer<TKey> comparer;

	private KeyCollection keys;

	private ValueCollection values;

	private object _syncRoot;

	private const string VersionName = "Version";

	private const string HashSizeName = "HashSize";

	private const string KeyValuePairsName = "KeyValuePairs";

	private const string ComparerName = "Comparer";

	public IEqualityComparer<TKey> Comparer => comparer;

	public int Count => count - freeCount;

	public KeyCollection Keys
	{
		get
		{
			if (keys == null)
			{
				keys = new KeyCollection(this);
			}
			return keys;
		}
	}

	ICollection<TKey> IDictionary<TKey, TValue>.Keys
	{
		get
		{
			if (keys == null)
			{
				keys = new KeyCollection(this);
			}
			return keys;
		}
	}

	IEnumerable<TKey> IReadOnlyDictionary<TKey, TValue>.Keys
	{
		get
		{
			if (keys == null)
			{
				keys = new KeyCollection(this);
			}
			return keys;
		}
	}

	public ValueCollection Values
	{
		get
		{
			if (values == null)
			{
				values = new ValueCollection(this);
			}
			return values;
		}
	}

	ICollection<TValue> IDictionary<TKey, TValue>.Values
	{
		get
		{
			if (values == null)
			{
				values = new ValueCollection(this);
			}
			return values;
		}
	}

	IEnumerable<TValue> IReadOnlyDictionary<TKey, TValue>.Values
	{
		get
		{
			if (values == null)
			{
				values = new ValueCollection(this);
			}
			return values;
		}
	}

	public TValue this[TKey key]
	{
		get
		{
			int num = FindEntry(key);
			if (num >= 0)
			{
				return entries[num].value;
			}
			throw new KeyNotFoundException();
		}
		set
		{
			TryInsert(key, value, InsertionBehavior.OverwriteExisting);
		}
	}

	bool ICollection<KeyValuePair<TKey, TValue>>.IsReadOnly => false;

	bool ICollection.IsSynchronized => false;

	object ICollection.SyncRoot
	{
		get
		{
			if (_syncRoot == null)
			{
				Interlocked.CompareExchange<object>(ref _syncRoot, new object(), (object)null);
			}
			return _syncRoot;
		}
	}

	bool IDictionary.IsFixedSize => false;

	bool IDictionary.IsReadOnly => false;

	ICollection IDictionary.Keys => Keys;

	ICollection IDictionary.Values => Values;

	object IDictionary.this[object key]
	{
		get
		{
			if (IsCompatibleKey(key))
			{
				int num = FindEntry((TKey)key);
				if (num >= 0)
				{
					return entries[num].value;
				}
			}
			return null;
		}
		set
		{
			if (key == null)
			{
				throw new ArgumentNullException("key");
			}
			if (value == null && default(TValue) != null)
			{
				throw new ArgumentNullException("value");
			}
			try
			{
				TKey key2 = (TKey)key;
				try
				{
					this[key2] = (TValue)value;
				}
				catch (InvalidCastException)
				{
					throw new ArgumentException(SR.Format("The value '{0}' is not of type '{1}' and cannot be used in this generic collection.", value, typeof(TValue)), "value");
				}
			}
			catch (InvalidCastException)
			{
				throw new ArgumentException(SR.Format("The value '{0}' is not of type '{1}' and cannot be used in this generic collection.", key, typeof(TKey)), "key");
			}
		}
	}

	public Dictionary()
		: this(0, (IEqualityComparer<TKey>)null)
	{
	}

	public Dictionary(int capacity)
		: this(capacity, (IEqualityComparer<TKey>)null)
	{
	}

	public Dictionary(IEqualityComparer<TKey> comparer)
		: this(0, comparer)
	{
	}

	public Dictionary(int capacity, IEqualityComparer<TKey> comparer)
	{
		if (capacity < 0)
		{
			throw new ArgumentOutOfRangeException("capacity", capacity, "Non-negative number required.");
		}
		if (capacity > 0)
		{
			Initialize(capacity);
		}
		this.comparer = comparer ?? EqualityComparer<TKey>.Default;
	}

	public Dictionary(IDictionary<TKey, TValue> dictionary)
		: this(dictionary, (IEqualityComparer<TKey>)null)
	{
	}

	public Dictionary(IDictionary<TKey, TValue> dictionary, IEqualityComparer<TKey> comparer)
		: this(dictionary?.Count ?? 0, comparer)
	{
		if (dictionary == null)
		{
			throw new ArgumentNullException("dictionary");
		}
		if (dictionary.GetType() == typeof(Dictionary<TKey, TValue>))
		{
			Dictionary<TKey, TValue> obj = (Dictionary<TKey, TValue>)dictionary;
			int num = obj.count;
			Entry[] array = obj.entries;
			for (int i = 0; i < num; i++)
			{
				if (array[i].hashCode >= 0)
				{
					Add(array[i].key, array[i].value);
				}
			}
			return;
		}
		foreach (KeyValuePair<TKey, TValue> item in dictionary)
		{
			Add(item.Key, item.Value);
		}
	}

	public Dictionary(IEnumerable<KeyValuePair<TKey, TValue>> collection)
		: this(collection, (IEqualityComparer<TKey>)null)
	{
	}

	public Dictionary(IEnumerable<KeyValuePair<TKey, TValue>> collection, IEqualityComparer<TKey> comparer)
		: this((collection as ICollection<KeyValuePair<TKey, TValue>>)?.Count ?? 0, comparer)
	{
		if (collection == null)
		{
			throw new ArgumentNullException("collection");
		}
		foreach (KeyValuePair<TKey, TValue> item in collection)
		{
			Add(item.Key, item.Value);
		}
	}

	protected Dictionary(SerializationInfo info, StreamingContext context)
	{
		DictionaryHashHelpers.SerializationInfoTable.Add(this, info);
	}

	public void Add(TKey key, TValue value)
	{
		TryInsert(key, value, InsertionBehavior.ThrowOnExisting);
	}

	void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> keyValuePair)
	{
		Add(keyValuePair.Key, keyValuePair.Value);
	}

	bool ICollection<KeyValuePair<TKey, TValue>>.Contains(KeyValuePair<TKey, TValue> keyValuePair)
	{
		int num = FindEntry(keyValuePair.Key);
		if (num >= 0 && EqualityComparer<TValue>.Default.Equals(entries[num].value, keyValuePair.Value))
		{
			return true;
		}
		return false;
	}

	bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> keyValuePair)
	{
		int num = FindEntry(keyValuePair.Key);
		if (num >= 0 && EqualityComparer<TValue>.Default.Equals(entries[num].value, keyValuePair.Value))
		{
			Remove(keyValuePair.Key);
			return true;
		}
		return false;
	}

	public void Clear()
	{
		if (count > 0)
		{
			for (int i = 0; i < buckets.Length; i++)
			{
				buckets[i] = -1;
			}
			Array.Clear(entries, 0, count);
			freeList = -1;
			count = 0;
			freeCount = 0;
			version++;
		}
	}

	public bool ContainsKey(TKey key)
	{
		return FindEntry(key) >= 0;
	}

	public bool ContainsValue(TValue value)
	{
		if (value == null)
		{
			for (int i = 0; i < count; i++)
			{
				if (entries[i].hashCode >= 0 && entries[i].value == null)
				{
					return true;
				}
			}
		}
		else
		{
			EqualityComparer<TValue> @default = EqualityComparer<TValue>.Default;
			for (int j = 0; j < count; j++)
			{
				if (entries[j].hashCode >= 0 && @default.Equals(entries[j].value, value))
				{
					return true;
				}
			}
		}
		return false;
	}

	private void CopyTo(KeyValuePair<TKey, TValue>[] array, int index)
	{
		if (array == null)
		{
			throw new ArgumentNullException("array");
		}
		if (index < 0 || index > array.Length)
		{
			throw new ArgumentOutOfRangeException("index", index, "Index was out of range. Must be non-negative and less than the size of the collection.");
		}
		if (array.Length - index < Count)
		{
			throw new ArgumentException("Destination array is not long enough to copy all the items in the collection. Check array index and length.");
		}
		int num = count;
		Entry[] array2 = entries;
		for (int i = 0; i < num; i++)
		{
			if (array2[i].hashCode >= 0)
			{
				array[index++] = new KeyValuePair<TKey, TValue>(array2[i].key, array2[i].value);
			}
		}
	}

	public Enumerator GetEnumerator()
	{
		return new Enumerator(this, 2);
	}

	IEnumerator<KeyValuePair<TKey, TValue>> IEnumerable<KeyValuePair<TKey, TValue>>.GetEnumerator()
	{
		return new Enumerator(this, 2);
	}

	[SecurityCritical]
	public virtual void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		info.AddValue("Version", version);
		info.AddValue("Comparer", comparer, typeof(IEqualityComparer<TKey>));
		info.AddValue("HashSize", (buckets != null) ? buckets.Length : 0);
		if (buckets != null)
		{
			KeyValuePair<TKey, TValue>[] array = new KeyValuePair<TKey, TValue>[Count];
			CopyTo(array, 0);
			info.AddValue("KeyValuePairs", array, typeof(KeyValuePair<TKey, TValue>[]));
		}
	}

	private int FindEntry(TKey key)
	{
		if (key == null)
		{
			throw new ArgumentNullException("key");
		}
		if (buckets != null)
		{
			int num = comparer.GetHashCode(key) & 0x7FFFFFFF;
			for (int num2 = buckets[num % buckets.Length]; num2 >= 0; num2 = entries[num2].next)
			{
				if (entries[num2].hashCode == num && comparer.Equals(entries[num2].key, key))
				{
					return num2;
				}
			}
		}
		return -1;
	}

	private void Initialize(int capacity)
	{
		int prime = HashHelpers.GetPrime(capacity);
		buckets = new int[prime];
		for (int i = 0; i < buckets.Length; i++)
		{
			buckets[i] = -1;
		}
		entries = new Entry[prime];
		freeList = -1;
	}

	private bool TryInsert(TKey key, TValue value, InsertionBehavior behavior)
	{
		if (key == null)
		{
			throw new ArgumentNullException("key");
		}
		if (buckets == null)
		{
			Initialize(0);
		}
		int num = comparer.GetHashCode(key) & 0x7FFFFFFF;
		int num2 = num % buckets.Length;
		int num3 = 0;
		for (int num4 = buckets[num2]; num4 >= 0; num4 = entries[num4].next)
		{
			if (entries[num4].hashCode == num && comparer.Equals(entries[num4].key, key))
			{
				switch (behavior)
				{
				case InsertionBehavior.OverwriteExisting:
					entries[num4].value = value;
					version++;
					return true;
				case InsertionBehavior.ThrowOnExisting:
					throw new ArgumentException(SR.Format("An item with the same key has already been added. Key: {0}", key));
				default:
					return false;
				}
			}
			num3++;
		}
		int num5;
		if (freeCount > 0)
		{
			num5 = freeList;
			freeList = entries[num5].next;
			freeCount--;
		}
		else
		{
			if (count == entries.Length)
			{
				Resize();
				num2 = num % buckets.Length;
			}
			num5 = count;
			count++;
		}
		entries[num5].hashCode = num;
		entries[num5].next = buckets[num2];
		entries[num5].key = key;
		entries[num5].value = value;
		buckets[num2] = num5;
		version++;
		if (num3 > 100 && comparer is NonRandomizedStringEqualityComparer)
		{
			comparer = (IEqualityComparer<TKey>)EqualityComparer<string>.Default;
			Resize(entries.Length, forceNewHashCodes: true);
		}
		return true;
	}

	public virtual void OnDeserialization(object sender)
	{
		DictionaryHashHelpers.SerializationInfoTable.TryGetValue(this, out var value);
		if (value == null)
		{
			return;
		}
		int @int = value.GetInt32("Version");
		int int2 = value.GetInt32("HashSize");
		comparer = (IEqualityComparer<TKey>)value.GetValue("Comparer", typeof(IEqualityComparer<TKey>));
		if (int2 != 0)
		{
			buckets = new int[int2];
			for (int i = 0; i < buckets.Length; i++)
			{
				buckets[i] = -1;
			}
			entries = new Entry[int2];
			freeList = -1;
			KeyValuePair<TKey, TValue>[] array = (KeyValuePair<TKey, TValue>[])value.GetValue("KeyValuePairs", typeof(KeyValuePair<TKey, TValue>[]));
			if (array == null)
			{
				throw new SerializationException("The keys for this dictionary are missing.");
			}
			for (int j = 0; j < array.Length; j++)
			{
				if (array[j].Key == null)
				{
					throw new SerializationException("One of the serialized keys is null.");
				}
				Add(array[j].Key, array[j].Value);
			}
		}
		else
		{
			buckets = null;
		}
		version = @int;
		DictionaryHashHelpers.SerializationInfoTable.Remove(this);
	}

	private void Resize()
	{
		Resize(HashHelpers.ExpandPrime(count), forceNewHashCodes: false);
	}

	private void Resize(int newSize, bool forceNewHashCodes)
	{
		int[] array = new int[newSize];
		for (int i = 0; i < array.Length; i++)
		{
			array[i] = -1;
		}
		Entry[] array2 = new Entry[newSize];
		Array.Copy(entries, 0, array2, 0, count);
		if (forceNewHashCodes)
		{
			for (int j = 0; j < count; j++)
			{
				if (array2[j].hashCode != -1)
				{
					array2[j].hashCode = comparer.GetHashCode(array2[j].key) & 0x7FFFFFFF;
				}
			}
		}
		for (int k = 0; k < count; k++)
		{
			if (array2[k].hashCode >= 0)
			{
				int num = array2[k].hashCode % newSize;
				array2[k].next = array[num];
				array[num] = k;
			}
		}
		buckets = array;
		entries = array2;
	}

	public bool Remove(TKey key)
	{
		if (key == null)
		{
			throw new ArgumentNullException("key");
		}
		if (buckets != null)
		{
			int num = comparer.GetHashCode(key) & 0x7FFFFFFF;
			int num2 = num % buckets.Length;
			int num3 = -1;
			for (int num4 = buckets[num2]; num4 >= 0; num4 = entries[num4].next)
			{
				if (entries[num4].hashCode == num && comparer.Equals(entries[num4].key, key))
				{
					if (num3 < 0)
					{
						buckets[num2] = entries[num4].next;
					}
					else
					{
						entries[num3].next = entries[num4].next;
					}
					entries[num4].hashCode = -1;
					entries[num4].next = freeList;
					entries[num4].key = default(TKey);
					entries[num4].value = default(TValue);
					freeList = num4;
					freeCount++;
					version++;
					return true;
				}
				num3 = num4;
			}
		}
		return false;
	}

	public bool Remove(TKey key, out TValue value)
	{
		if (key == null)
		{
			throw new ArgumentNullException("key");
		}
		if (buckets != null)
		{
			int num = comparer.GetHashCode(key) & 0x7FFFFFFF;
			int num2 = num % buckets.Length;
			int num3 = -1;
			for (int num4 = buckets[num2]; num4 >= 0; num4 = entries[num4].next)
			{
				if (entries[num4].hashCode == num && comparer.Equals(entries[num4].key, key))
				{
					if (num3 < 0)
					{
						buckets[num2] = entries[num4].next;
					}
					else
					{
						entries[num3].next = entries[num4].next;
					}
					value = entries[num4].value;
					entries[num4].hashCode = -1;
					entries[num4].next = freeList;
					entries[num4].key = default(TKey);
					entries[num4].value = default(TValue);
					freeList = num4;
					freeCount++;
					version++;
					return true;
				}
				num3 = num4;
			}
		}
		value = default(TValue);
		return false;
	}

	public bool TryGetValue(TKey key, out TValue value)
	{
		int num = FindEntry(key);
		if (num >= 0)
		{
			value = entries[num].value;
			return true;
		}
		value = default(TValue);
		return false;
	}

	public bool TryAdd(TKey key, TValue value)
	{
		return TryInsert(key, value, InsertionBehavior.None);
	}

	void ICollection<KeyValuePair<TKey, TValue>>.CopyTo(KeyValuePair<TKey, TValue>[] array, int index)
	{
		CopyTo(array, index);
	}

	void ICollection.CopyTo(Array array, int index)
	{
		if (array == null)
		{
			throw new ArgumentNullException("array");
		}
		if (array.Rank != 1)
		{
			throw new ArgumentException("Only single dimensional arrays are supported for the requested action.", "array");
		}
		if (array.GetLowerBound(0) != 0)
		{
			throw new ArgumentException("The lower bound of target array must be zero.", "array");
		}
		if (index < 0 || index > array.Length)
		{
			throw new ArgumentOutOfRangeException("index", index, "Index was out of range. Must be non-negative and less than the size of the collection.");
		}
		if (array.Length - index < Count)
		{
			throw new ArgumentException("Destination array is not long enough to copy all the items in the collection. Check array index and length.");
		}
		if (array is KeyValuePair<TKey, TValue>[] array2)
		{
			CopyTo(array2, index);
			return;
		}
		if (array is DictionaryEntry[])
		{
			DictionaryEntry[] array3 = array as DictionaryEntry[];
			Entry[] array4 = entries;
			for (int i = 0; i < count; i++)
			{
				if (array4[i].hashCode >= 0)
				{
					array3[index++] = new DictionaryEntry(array4[i].key, array4[i].value);
				}
			}
			return;
		}
		if (!(array is object[] array5))
		{
			throw new ArgumentException("Target array type is not compatible with the type of items in the collection.", "array");
		}
		try
		{
			int num = count;
			Entry[] array6 = entries;
			for (int j = 0; j < num; j++)
			{
				if (array6[j].hashCode >= 0)
				{
					array5[index++] = new KeyValuePair<TKey, TValue>(array6[j].key, array6[j].value);
				}
			}
		}
		catch (ArrayTypeMismatchException)
		{
			throw new ArgumentException("Target array type is not compatible with the type of items in the collection.", "array");
		}
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return new Enumerator(this, 2);
	}

	private static bool IsCompatibleKey(object key)
	{
		if (key == null)
		{
			throw new ArgumentNullException("key");
		}
		return key is TKey;
	}

	void IDictionary.Add(object key, object value)
	{
		if (key == null)
		{
			throw new ArgumentNullException("key");
		}
		if (value == null && default(TValue) != null)
		{
			throw new ArgumentNullException("value");
		}
		try
		{
			TKey key2 = (TKey)key;
			try
			{
				Add(key2, (TValue)value);
			}
			catch (InvalidCastException)
			{
				throw new ArgumentException(SR.Format("The value '{0}' is not of type '{1}' and cannot be used in this generic collection.", value, typeof(TValue)), "value");
			}
		}
		catch (InvalidCastException)
		{
			throw new ArgumentException(SR.Format("The value '{0}' is not of type '{1}' and cannot be used in this generic collection.", key, typeof(TKey)), "key");
		}
	}

	bool IDictionary.Contains(object key)
	{
		if (IsCompatibleKey(key))
		{
			return ContainsKey((TKey)key);
		}
		return false;
	}

	IDictionaryEnumerator IDictionary.GetEnumerator()
	{
		return new Enumerator(this, 1);
	}

	void IDictionary.Remove(object key)
	{
		if (IsCompatibleKey(key))
		{
			Remove((TKey)key);
		}
	}
}

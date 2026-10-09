using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using Unity;

namespace System.Collections.ObjectModel;

[Serializable]
[DebuggerTypeProxy(typeof(Mscorlib_DictionaryDebugView<, >))]
[DebuggerDisplay("Count = {Count}")]
public class ReadOnlyDictionary<TKey, TValue> : IDictionary<TKey, TValue>, ICollection<KeyValuePair<TKey, TValue>>, IEnumerable<KeyValuePair<TKey, TValue>>, IEnumerable, IDictionary, ICollection, IReadOnlyDictionary<TKey, TValue>, IReadOnlyCollection<KeyValuePair<TKey, TValue>>
{
	[Serializable]
	private struct DictionaryEnumerator : IDictionaryEnumerator, IEnumerator
	{
		private readonly IDictionary<TKey, TValue> m_dictionary;

		private IEnumerator<KeyValuePair<TKey, TValue>> m_enumerator;

		public DictionaryEntry Entry => new DictionaryEntry(m_enumerator.Current.Key, m_enumerator.Current.Value);

		public object Key => m_enumerator.Current.Key;

		public object Value => m_enumerator.Current.Value;

		public object Current => Entry;

		public DictionaryEnumerator(IDictionary<TKey, TValue> dictionary)
		{
			m_dictionary = dictionary;
			m_enumerator = m_dictionary.GetEnumerator();
		}

		public bool MoveNext()
		{
			return m_enumerator.MoveNext();
		}

		public void Reset()
		{
			m_enumerator.Reset();
		}
	}

	[Serializable]
	[DebuggerDisplay("Count = {Count}")]
	[DebuggerTypeProxy(typeof(Mscorlib_CollectionDebugView<>))]
	public sealed class KeyCollection : ICollection<TKey>, IEnumerable<TKey>, IEnumerable, ICollection, IReadOnlyCollection<TKey>
	{
		private readonly ICollection<TKey> m_collection;

		[NonSerialized]
		private object m_syncRoot;

		public int Count => m_collection.Count;

		bool ICollection<TKey>.IsReadOnly => true;

		bool ICollection.IsSynchronized => false;

		object ICollection.SyncRoot
		{
			get
			{
				if (m_syncRoot == null)
				{
					if (m_collection is ICollection collection)
					{
						m_syncRoot = collection.SyncRoot;
					}
					else
					{
						Interlocked.CompareExchange<object>(ref m_syncRoot, new object(), (object)null);
					}
				}
				return m_syncRoot;
			}
		}

		internal KeyCollection(ICollection<TKey> collection)
		{
			if (collection == null)
			{
				ThrowHelper.ThrowArgumentNullException(ExceptionArgument.collection);
			}
			m_collection = collection;
		}

		void ICollection<TKey>.Add(TKey item)
		{
			ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
		}

		void ICollection<TKey>.Clear()
		{
			ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
		}

		bool ICollection<TKey>.Contains(TKey item)
		{
			return m_collection.Contains(item);
		}

		public void CopyTo(TKey[] array, int arrayIndex)
		{
			m_collection.CopyTo(array, arrayIndex);
		}

		bool ICollection<TKey>.Remove(TKey item)
		{
			ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
			return false;
		}

		public IEnumerator<TKey> GetEnumerator()
		{
			return m_collection.GetEnumerator();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return ((IEnumerable)m_collection).GetEnumerator();
		}

		void ICollection.CopyTo(Array array, int index)
		{
			ReadOnlyDictionaryHelpers.CopyToNonGenericICollectionHelper(m_collection, array, index);
		}

		internal KeyCollection()
		{
			ThrowStub.ThrowNotSupportedException();
		}
	}

	[Serializable]
	[DebuggerDisplay("Count = {Count}")]
	[DebuggerTypeProxy(typeof(Mscorlib_CollectionDebugView<>))]
	public sealed class ValueCollection : ICollection<TValue>, IEnumerable<TValue>, IEnumerable, ICollection, IReadOnlyCollection<TValue>
	{
		private readonly ICollection<TValue> m_collection;

		[NonSerialized]
		private object m_syncRoot;

		public int Count => m_collection.Count;

		bool ICollection<TValue>.IsReadOnly => true;

		bool ICollection.IsSynchronized => false;

		object ICollection.SyncRoot
		{
			get
			{
				if (m_syncRoot == null)
				{
					if (m_collection is ICollection collection)
					{
						m_syncRoot = collection.SyncRoot;
					}
					else
					{
						Interlocked.CompareExchange<object>(ref m_syncRoot, new object(), (object)null);
					}
				}
				return m_syncRoot;
			}
		}

		internal ValueCollection(ICollection<TValue> collection)
		{
			if (collection == null)
			{
				ThrowHelper.ThrowArgumentNullException(ExceptionArgument.collection);
			}
			m_collection = collection;
		}

		void ICollection<TValue>.Add(TValue item)
		{
			ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
		}

		void ICollection<TValue>.Clear()
		{
			ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
		}

		bool ICollection<TValue>.Contains(TValue item)
		{
			return m_collection.Contains(item);
		}

		public void CopyTo(TValue[] array, int arrayIndex)
		{
			m_collection.CopyTo(array, arrayIndex);
		}

		bool ICollection<TValue>.Remove(TValue item)
		{
			ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
			return false;
		}

		public IEnumerator<TValue> GetEnumerator()
		{
			return m_collection.GetEnumerator();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return ((IEnumerable)m_collection).GetEnumerator();
		}

		void ICollection.CopyTo(Array array, int index)
		{
			ReadOnlyDictionaryHelpers.CopyToNonGenericICollectionHelper(m_collection, array, index);
		}

		internal ValueCollection()
		{
			ThrowStub.ThrowNotSupportedException();
		}
	}

	private readonly IDictionary<TKey, TValue> m_dictionary;

	[NonSerialized]
	private object m_syncRoot;

	[NonSerialized]
	private KeyCollection m_keys;

	[NonSerialized]
	private ValueCollection m_values;

	protected IDictionary<TKey, TValue> Dictionary => m_dictionary;

	public KeyCollection Keys
	{
		get
		{
			if (m_keys == null)
			{
				m_keys = new KeyCollection(m_dictionary.Keys);
			}
			return m_keys;
		}
	}

	public ValueCollection Values
	{
		get
		{
			if (m_values == null)
			{
				m_values = new ValueCollection(m_dictionary.Values);
			}
			return m_values;
		}
	}

	ICollection<TKey> IDictionary<TKey, TValue>.Keys => Keys;

	ICollection<TValue> IDictionary<TKey, TValue>.Values => Values;

	public TValue this[TKey key] => m_dictionary[key];

	TValue IDictionary<TKey, TValue>.this[TKey key]
	{
		get
		{
			return m_dictionary[key];
		}
		set
		{
			ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
		}
	}

	public int Count => m_dictionary.Count;

	bool ICollection<KeyValuePair<TKey, TValue>>.IsReadOnly => true;

	bool IDictionary.IsFixedSize => true;

	bool IDictionary.IsReadOnly => true;

	ICollection IDictionary.Keys => Keys;

	ICollection IDictionary.Values => Values;

	object IDictionary.this[object key]
	{
		get
		{
			if (IsCompatibleKey(key))
			{
				return this[(TKey)key];
			}
			return null;
		}
		set
		{
			ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
		}
	}

	bool ICollection.IsSynchronized => false;

	object ICollection.SyncRoot
	{
		get
		{
			if (m_syncRoot == null)
			{
				if (m_dictionary is ICollection collection)
				{
					m_syncRoot = collection.SyncRoot;
				}
				else
				{
					Interlocked.CompareExchange<object>(ref m_syncRoot, new object(), (object)null);
				}
			}
			return m_syncRoot;
		}
	}

	IEnumerable<TKey> IReadOnlyDictionary<TKey, TValue>.Keys => Keys;

	IEnumerable<TValue> IReadOnlyDictionary<TKey, TValue>.Values => Values;

	public ReadOnlyDictionary(IDictionary<TKey, TValue> dictionary)
	{
		if (dictionary == null)
		{
			throw new ArgumentNullException("dictionary");
		}
		m_dictionary = dictionary;
	}

	public bool ContainsKey(TKey key)
	{
		return m_dictionary.ContainsKey(key);
	}

	public bool TryGetValue(TKey key, out TValue value)
	{
		return m_dictionary.TryGetValue(key, out value);
	}

	void IDictionary<TKey, TValue>.Add(TKey key, TValue value)
	{
		ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
	}

	bool IDictionary<TKey, TValue>.Remove(TKey key)
	{
		ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
		return false;
	}

	bool ICollection<KeyValuePair<TKey, TValue>>.Contains(KeyValuePair<TKey, TValue> item)
	{
		return m_dictionary.Contains(item);
	}

	void ICollection<KeyValuePair<TKey, TValue>>.CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
	{
		m_dictionary.CopyTo(array, arrayIndex);
	}

	void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> item)
	{
		ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
	}

	void ICollection<KeyValuePair<TKey, TValue>>.Clear()
	{
		ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
	}

	bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> item)
	{
		ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
		return false;
	}

	public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
	{
		return m_dictionary.GetEnumerator();
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return ((IEnumerable)m_dictionary).GetEnumerator();
	}

	private static bool IsCompatibleKey(object key)
	{
		if (key == null)
		{
			ThrowHelper.ThrowArgumentNullException(ExceptionArgument.key);
		}
		return key is TKey;
	}

	void IDictionary.Add(object key, object value)
	{
		ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
	}

	void IDictionary.Clear()
	{
		ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
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
		if (m_dictionary is IDictionary dictionary)
		{
			return dictionary.GetEnumerator();
		}
		return new DictionaryEnumerator(m_dictionary);
	}

	void IDictionary.Remove(object key)
	{
		ThrowHelper.ThrowNotSupportedException(ExceptionResource.NotSupported_ReadOnlyCollection);
	}

	void ICollection.CopyTo(Array array, int index)
	{
		if (array == null)
		{
			ThrowHelper.ThrowArgumentNullException(ExceptionArgument.array);
		}
		if (array.Rank != 1)
		{
			ThrowHelper.ThrowArgumentException(ExceptionResource.Arg_RankMultiDimNotSupported);
		}
		if (array.GetLowerBound(0) != 0)
		{
			ThrowHelper.ThrowArgumentException(ExceptionResource.Arg_NonZeroLowerBound);
		}
		if (index < 0 || index > array.Length)
		{
			ThrowHelper.ThrowArgumentOutOfRangeException(ExceptionArgument.index, ExceptionResource.ArgumentOutOfRange_NeedNonNegNum);
		}
		if (array.Length - index < Count)
		{
			ThrowHelper.ThrowArgumentException(ExceptionResource.Arg_ArrayPlusOffTooSmall);
		}
		if (array is KeyValuePair<TKey, TValue>[] array2)
		{
			m_dictionary.CopyTo(array2, index);
			return;
		}
		if (array is DictionaryEntry[] array3)
		{
			{
				foreach (KeyValuePair<TKey, TValue> item in m_dictionary)
				{
					array3[index++] = new DictionaryEntry(item.Key, item.Value);
				}
				return;
			}
		}
		object[] array4 = array as object[];
		if (array4 == null)
		{
			ThrowHelper.ThrowArgumentException(ExceptionResource.Argument_InvalidArrayType);
		}
		try
		{
			foreach (KeyValuePair<TKey, TValue> item2 in m_dictionary)
			{
				array4[index++] = new KeyValuePair<TKey, TValue>(item2.Key, item2.Value);
			}
		}
		catch (ArrayTypeMismatchException)
		{
			ThrowHelper.ThrowArgumentException(ExceptionResource.Argument_InvalidArrayType);
		}
	}
}

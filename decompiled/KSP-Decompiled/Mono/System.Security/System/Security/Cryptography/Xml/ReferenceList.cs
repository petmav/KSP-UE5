using System.Collections;
using System.Runtime.CompilerServices;

namespace System.Security.Cryptography.Xml;

public sealed class ReferenceList : IList, ICollection, IEnumerable
{
	private ArrayList _references;

	public int Count => _references.Count;

	[IndexerName("ItemOf")]
	public EncryptedReference this[int index]
	{
		get
		{
			return Item(index);
		}
		set
		{
			((IList)this)[index] = value;
		}
	}

	object IList.this[int index]
	{
		get
		{
			return _references[index];
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			if (!(value is DataReference) && !(value is KeyReference))
			{
				throw new ArgumentException("Type of input object is invalid.", "value");
			}
			_references[index] = value;
		}
	}

	bool IList.IsFixedSize => _references.IsFixedSize;

	bool IList.IsReadOnly => _references.IsReadOnly;

	public object SyncRoot => _references.SyncRoot;

	public bool IsSynchronized => _references.IsSynchronized;

	public ReferenceList()
	{
		_references = new ArrayList();
	}

	public IEnumerator GetEnumerator()
	{
		return _references.GetEnumerator();
	}

	public int Add(object value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (!(value is DataReference) && !(value is KeyReference))
		{
			throw new ArgumentException("Type of input object is invalid.", "value");
		}
		return _references.Add(value);
	}

	public void Clear()
	{
		_references.Clear();
	}

	public bool Contains(object value)
	{
		return _references.Contains(value);
	}

	public int IndexOf(object value)
	{
		return _references.IndexOf(value);
	}

	public void Insert(int index, object value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (!(value is DataReference) && !(value is KeyReference))
		{
			throw new ArgumentException("Type of input object is invalid.", "value");
		}
		_references.Insert(index, value);
	}

	public void Remove(object value)
	{
		_references.Remove(value);
	}

	public void RemoveAt(int index)
	{
		_references.RemoveAt(index);
	}

	public EncryptedReference Item(int index)
	{
		return (EncryptedReference)_references[index];
	}

	public void CopyTo(Array array, int index)
	{
		_references.CopyTo(array, index);
	}
}

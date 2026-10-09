using System.Collections;
using System.Runtime.CompilerServices;

namespace System.Security.Cryptography.Xml;

public sealed class EncryptionPropertyCollection : IList, ICollection, IEnumerable
{
	private ArrayList _props;

	public int Count => _props.Count;

	public bool IsFixedSize => _props.IsFixedSize;

	public bool IsReadOnly => _props.IsReadOnly;

	[IndexerName("ItemOf")]
	public EncryptionProperty this[int index]
	{
		get
		{
			return (EncryptionProperty)((IList)this)[index];
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
			return _props[index];
		}
		set
		{
			if (!(value is EncryptionProperty))
			{
				throw new ArgumentException("Type of input object is invalid.", "value");
			}
			_props[index] = value;
		}
	}

	public object SyncRoot => _props.SyncRoot;

	public bool IsSynchronized => _props.IsSynchronized;

	public EncryptionPropertyCollection()
	{
		_props = new ArrayList();
	}

	public IEnumerator GetEnumerator()
	{
		return _props.GetEnumerator();
	}

	int IList.Add(object value)
	{
		if (!(value is EncryptionProperty))
		{
			throw new ArgumentException("Type of input object is invalid.", "value");
		}
		return _props.Add(value);
	}

	public int Add(EncryptionProperty value)
	{
		return _props.Add(value);
	}

	public void Clear()
	{
		_props.Clear();
	}

	bool IList.Contains(object value)
	{
		if (!(value is EncryptionProperty))
		{
			throw new ArgumentException("Type of input object is invalid.", "value");
		}
		return _props.Contains(value);
	}

	public bool Contains(EncryptionProperty value)
	{
		return _props.Contains(value);
	}

	int IList.IndexOf(object value)
	{
		if (!(value is EncryptionProperty))
		{
			throw new ArgumentException("Type of input object is invalid.", "value");
		}
		return _props.IndexOf(value);
	}

	public int IndexOf(EncryptionProperty value)
	{
		return _props.IndexOf(value);
	}

	void IList.Insert(int index, object value)
	{
		if (!(value is EncryptionProperty))
		{
			throw new ArgumentException("Type of input object is invalid.", "value");
		}
		_props.Insert(index, value);
	}

	public void Insert(int index, EncryptionProperty value)
	{
		_props.Insert(index, value);
	}

	void IList.Remove(object value)
	{
		if (!(value is EncryptionProperty))
		{
			throw new ArgumentException("Type of input object is invalid.", "value");
		}
		_props.Remove(value);
	}

	public void Remove(EncryptionProperty value)
	{
		_props.Remove(value);
	}

	public void RemoveAt(int index)
	{
		_props.RemoveAt(index);
	}

	public EncryptionProperty Item(int index)
	{
		return (EncryptionProperty)_props[index];
	}

	public void CopyTo(Array array, int index)
	{
		_props.CopyTo(array, index);
	}

	public void CopyTo(EncryptionProperty[] array, int index)
	{
		_props.CopyTo(array, index);
	}
}

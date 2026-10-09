using System.Collections;
using System.Collections.Generic;

namespace System.CodeDom;

[Serializable]
public class CodeNamespaceImportCollection : IList, ICollection, IEnumerable
{
	private readonly ArrayList _data = new ArrayList();

	private readonly Dictionary<string, CodeNamespaceImport> _keys = new Dictionary<string, CodeNamespaceImport>(StringComparer.OrdinalIgnoreCase);

	public CodeNamespaceImport this[int index]
	{
		get
		{
			return (CodeNamespaceImport)_data[index];
		}
		set
		{
			_data[index] = value;
			SyncKeys();
		}
	}

	public int Count => _data.Count;

	bool IList.IsReadOnly => false;

	bool IList.IsFixedSize => false;

	object IList.this[int index]
	{
		get
		{
			return this[index];
		}
		set
		{
			this[index] = (CodeNamespaceImport)value;
			SyncKeys();
		}
	}

	int ICollection.Count => Count;

	bool ICollection.IsSynchronized => false;

	object ICollection.SyncRoot => null;

	public void Add(CodeNamespaceImport value)
	{
		if (!_keys.ContainsKey(value.Namespace))
		{
			_keys[value.Namespace] = value;
			_data.Add(value);
		}
	}

	public void AddRange(CodeNamespaceImport[] value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		foreach (CodeNamespaceImport value2 in value)
		{
			Add(value2);
		}
	}

	public void Clear()
	{
		_data.Clear();
		_keys.Clear();
	}

	private void SyncKeys()
	{
		_keys.Clear();
		foreach (CodeNamespaceImport datum in _data)
		{
			_keys[datum.Namespace] = datum;
		}
	}

	public IEnumerator GetEnumerator()
	{
		return _data.GetEnumerator();
	}

	void ICollection.CopyTo(Array array, int index)
	{
		_data.CopyTo(array, index);
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return GetEnumerator();
	}

	int IList.Add(object value)
	{
		return _data.Add((CodeNamespaceImport)value);
	}

	void IList.Clear()
	{
		Clear();
	}

	bool IList.Contains(object value)
	{
		return _data.Contains(value);
	}

	int IList.IndexOf(object value)
	{
		return _data.IndexOf((CodeNamespaceImport)value);
	}

	void IList.Insert(int index, object value)
	{
		_data.Insert(index, (CodeNamespaceImport)value);
		SyncKeys();
	}

	void IList.Remove(object value)
	{
		_data.Remove((CodeNamespaceImport)value);
		SyncKeys();
	}

	void IList.RemoveAt(int index)
	{
		_data.RemoveAt(index);
		SyncKeys();
	}
}

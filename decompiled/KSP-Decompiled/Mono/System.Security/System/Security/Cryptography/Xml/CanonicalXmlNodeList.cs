using System.Collections;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal class CanonicalXmlNodeList : XmlNodeList, IList, ICollection, IEnumerable
{
	private ArrayList _nodeArray;

	public override int Count => _nodeArray.Count;

	public bool IsFixedSize => _nodeArray.IsFixedSize;

	public bool IsReadOnly => _nodeArray.IsReadOnly;

	object IList.this[int index]
	{
		get
		{
			return _nodeArray[index];
		}
		set
		{
			if (!(value is XmlNode))
			{
				throw new ArgumentException("Type of input object is invalid.", "value");
			}
			_nodeArray[index] = value;
		}
	}

	public object SyncRoot => _nodeArray.SyncRoot;

	public bool IsSynchronized => _nodeArray.IsSynchronized;

	internal CanonicalXmlNodeList()
	{
		_nodeArray = new ArrayList();
	}

	public override XmlNode Item(int index)
	{
		return (XmlNode)_nodeArray[index];
	}

	public override IEnumerator GetEnumerator()
	{
		return _nodeArray.GetEnumerator();
	}

	public int Add(object value)
	{
		if (!(value is XmlNode))
		{
			throw new ArgumentException("Type of input object is invalid.", "node");
		}
		return _nodeArray.Add(value);
	}

	public void Clear()
	{
		_nodeArray.Clear();
	}

	public bool Contains(object value)
	{
		return _nodeArray.Contains(value);
	}

	public int IndexOf(object value)
	{
		return _nodeArray.IndexOf(value);
	}

	public void Insert(int index, object value)
	{
		if (!(value is XmlNode))
		{
			throw new ArgumentException("Type of input object is invalid.", "value");
		}
		_nodeArray.Insert(index, value);
	}

	public void Remove(object value)
	{
		_nodeArray.Remove(value);
	}

	public void RemoveAt(int index)
	{
		_nodeArray.RemoveAt(index);
	}

	public void CopyTo(Array array, int index)
	{
		_nodeArray.CopyTo(array, index);
	}
}

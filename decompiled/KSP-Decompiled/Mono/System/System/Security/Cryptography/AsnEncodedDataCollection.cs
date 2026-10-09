using System.Collections;

namespace System.Security.Cryptography;

public sealed class AsnEncodedDataCollection : ICollection, IEnumerable
{
	private ArrayList _list;

	public int Count => _list.Count;

	public bool IsSynchronized => _list.IsSynchronized;

	public AsnEncodedData this[int index] => (AsnEncodedData)_list[index];

	public object SyncRoot => _list.SyncRoot;

	public AsnEncodedDataCollection()
	{
		_list = new ArrayList();
	}

	public AsnEncodedDataCollection(AsnEncodedData asnEncodedData)
	{
		_list = new ArrayList();
		_list.Add(asnEncodedData);
	}

	public int Add(AsnEncodedData asnEncodedData)
	{
		return _list.Add(asnEncodedData);
	}

	public void CopyTo(AsnEncodedData[] array, int index)
	{
		_list.CopyTo(array, index);
	}

	void ICollection.CopyTo(Array array, int index)
	{
		_list.CopyTo(array, index);
	}

	public AsnEncodedDataEnumerator GetEnumerator()
	{
		return new AsnEncodedDataEnumerator(this);
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return new AsnEncodedDataEnumerator(this);
	}

	public void Remove(AsnEncodedData asnEncodedData)
	{
		_list.Remove(asnEncodedData);
	}
}

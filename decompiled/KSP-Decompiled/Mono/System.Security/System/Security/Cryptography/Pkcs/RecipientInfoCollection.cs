using System.Collections;

namespace System.Security.Cryptography.Pkcs;

public sealed class RecipientInfoCollection : ICollection, IEnumerable
{
	private ArrayList _list;

	public int Count => _list.Count;

	public bool IsSynchronized => _list.IsSynchronized;

	public RecipientInfo this[int index] => (RecipientInfo)_list[index];

	public object SyncRoot => _list.SyncRoot;

	internal RecipientInfoCollection()
	{
		_list = new ArrayList();
	}

	internal int Add(RecipientInfo ri)
	{
		return _list.Add(ri);
	}

	public void CopyTo(Array array, int index)
	{
		_list.CopyTo(array, index);
	}

	public void CopyTo(RecipientInfo[] array, int index)
	{
		_list.CopyTo(array, index);
	}

	public RecipientInfoEnumerator GetEnumerator()
	{
		return new RecipientInfoEnumerator(_list);
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return new RecipientInfoEnumerator(_list);
	}
}

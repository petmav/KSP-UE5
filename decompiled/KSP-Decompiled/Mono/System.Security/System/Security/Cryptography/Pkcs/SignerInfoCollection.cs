using System.Collections;

namespace System.Security.Cryptography.Pkcs;

public sealed class SignerInfoCollection : ICollection, IEnumerable
{
	private ArrayList _list;

	public int Count => _list.Count;

	public bool IsSynchronized => false;

	public SignerInfo this[int index] => (SignerInfo)_list[index];

	public object SyncRoot => _list.SyncRoot;

	internal SignerInfoCollection()
	{
		_list = new ArrayList();
	}

	internal void Add(SignerInfo signer)
	{
		_list.Add(signer);
	}

	public void CopyTo(Array array, int index)
	{
		if (array == null)
		{
			throw new ArgumentNullException("array");
		}
		if (index < 0 || index >= array.Length)
		{
			throw new ArgumentOutOfRangeException("index");
		}
		_list.CopyTo(array, index);
	}

	public void CopyTo(SignerInfo[] array, int index)
	{
		if (array == null)
		{
			throw new ArgumentNullException("array");
		}
		if (index < 0 || index >= array.Length)
		{
			throw new ArgumentOutOfRangeException("index");
		}
		_list.CopyTo(array, index);
	}

	public SignerInfoEnumerator GetEnumerator()
	{
		return new SignerInfoEnumerator(_list);
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return new SignerInfoEnumerator(_list);
	}
}

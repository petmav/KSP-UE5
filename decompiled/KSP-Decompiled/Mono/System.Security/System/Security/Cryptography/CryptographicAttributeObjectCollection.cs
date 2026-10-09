using System.Collections;

namespace System.Security.Cryptography;

public sealed class CryptographicAttributeObjectCollection : ICollection, IEnumerable
{
	private ArrayList _list;

	public int Count => _list.Count;

	public bool IsSynchronized => _list.IsSynchronized;

	public CryptographicAttributeObject this[int index] => (CryptographicAttributeObject)_list[index];

	public object SyncRoot => this;

	public CryptographicAttributeObjectCollection()
	{
		_list = new ArrayList();
	}

	public CryptographicAttributeObjectCollection(CryptographicAttributeObject attribute)
		: this()
	{
		_list.Add(attribute);
	}

	public int Add(AsnEncodedData asnEncodedData)
	{
		if (asnEncodedData == null)
		{
			throw new ArgumentNullException("asnEncodedData");
		}
		AsnEncodedDataCollection values = new AsnEncodedDataCollection(asnEncodedData);
		return Add(new CryptographicAttributeObject(asnEncodedData.Oid, values));
	}

	public int Add(CryptographicAttributeObject attribute)
	{
		if (attribute == null)
		{
			throw new ArgumentNullException("attribute");
		}
		int num = -1;
		string value = attribute.Oid.Value;
		for (int i = 0; i < _list.Count; i++)
		{
			if ((_list[i] as CryptographicAttributeObject).Oid.Value == value)
			{
				num = i;
				break;
			}
		}
		if (num >= 0)
		{
			CryptographicAttributeObject cryptographicAttributeObject = this[num];
			AsnEncodedDataEnumerator enumerator = attribute.Values.GetEnumerator();
			while (enumerator.MoveNext())
			{
				AsnEncodedData current = enumerator.Current;
				cryptographicAttributeObject.Values.Add(current);
			}
			return num;
		}
		return _list.Add(attribute);
	}

	public void CopyTo(CryptographicAttributeObject[] array, int index)
	{
		_list.CopyTo(array, index);
	}

	void ICollection.CopyTo(Array array, int index)
	{
		_list.CopyTo(array, index);
	}

	public CryptographicAttributeObjectEnumerator GetEnumerator()
	{
		return new CryptographicAttributeObjectEnumerator(_list);
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return new CryptographicAttributeObjectEnumerator(_list);
	}

	public void Remove(CryptographicAttributeObject attribute)
	{
		if (attribute == null)
		{
			throw new ArgumentNullException("attribute");
		}
		_list.Remove(attribute);
	}
}

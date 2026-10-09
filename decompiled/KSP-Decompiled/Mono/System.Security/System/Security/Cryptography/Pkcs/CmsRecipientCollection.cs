using System.Collections;
using System.Security.Cryptography.X509Certificates;

namespace System.Security.Cryptography.Pkcs;

public sealed class CmsRecipientCollection : ICollection, IEnumerable
{
	private ArrayList _list;

	public int Count => _list.Count;

	public bool IsSynchronized => _list.IsSynchronized;

	public CmsRecipient this[int index] => (CmsRecipient)_list[index];

	public object SyncRoot => _list.SyncRoot;

	public CmsRecipientCollection()
	{
		_list = new ArrayList();
	}

	public CmsRecipientCollection(CmsRecipient recipient)
	{
		_list.Add(recipient);
	}

	public CmsRecipientCollection(SubjectIdentifierType recipientIdentifierType, X509Certificate2Collection certificates)
	{
		X509Certificate2Enumerator enumerator = certificates.GetEnumerator();
		while (enumerator.MoveNext())
		{
			X509Certificate2 current = enumerator.Current;
			CmsRecipient value = new CmsRecipient(recipientIdentifierType, current);
			_list.Add(value);
		}
	}

	public int Add(CmsRecipient recipient)
	{
		return _list.Add(recipient);
	}

	public void CopyTo(Array array, int index)
	{
		_list.CopyTo(array, index);
	}

	public void CopyTo(CmsRecipient[] array, int index)
	{
		_list.CopyTo(array, index);
	}

	public CmsRecipientEnumerator GetEnumerator()
	{
		return new CmsRecipientEnumerator(_list);
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return new CmsRecipientEnumerator(_list);
	}

	public void Remove(CmsRecipient recipient)
	{
		_list.Remove(recipient);
	}
}

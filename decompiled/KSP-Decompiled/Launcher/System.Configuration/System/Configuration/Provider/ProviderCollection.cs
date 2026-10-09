using System.Collections;
using Unity;

namespace System.Configuration.Provider;

public class ProviderCollection : ICollection, IEnumerable
{
	public int Count
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public bool IsSynchronized
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public ProviderBase this[string name]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public object SyncRoot
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ProviderCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void Add(ProviderBase provider)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyTo(ProviderBase[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Remove(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetReadOnly()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	void ICollection.CopyTo(Array array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

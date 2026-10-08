using System.Collections;
using Unity;

namespace System.Configuration;

public class SettingsPropertyCollection : ICollection, IEnumerable, ICloneable
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

	public SettingsProperty this[string name]
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

	public SettingsPropertyCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(SettingsProperty property)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void CopyTo(Array array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual void OnAdd(SettingsProperty property)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnAddComplete(SettingsProperty property)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnClear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnClearComplete()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnRemove(SettingsProperty property)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnRemoveComplete(SettingsProperty property)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetReadOnly()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

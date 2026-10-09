using System.Collections;

namespace System.Configuration;

public class SettingsPropertyCollection : ICloneable, ICollection, IEnumerable
{
	private Hashtable items;

	private bool isReadOnly;

	public int Count => items.Count;

	public bool IsSynchronized => false;

	public SettingsProperty this[string name] => (SettingsProperty)items[name];

	public object SyncRoot => this;

	public SettingsPropertyCollection()
	{
		items = new Hashtable();
	}

	public void Add(SettingsProperty property)
	{
		if (isReadOnly)
		{
			throw new NotSupportedException();
		}
		OnAdd(property);
		items.Add(property.Name, property);
		OnAddComplete(property);
	}

	public void Clear()
	{
		if (isReadOnly)
		{
			throw new NotSupportedException();
		}
		OnClear();
		items.Clear();
		OnClearComplete();
	}

	public object Clone()
	{
		return new SettingsPropertyCollection
		{
			items = (Hashtable)items.Clone()
		};
	}

	public void CopyTo(Array array, int index)
	{
		items.Values.CopyTo(array, index);
	}

	public IEnumerator GetEnumerator()
	{
		return items.Values.GetEnumerator();
	}

	public void Remove(string name)
	{
		if (isReadOnly)
		{
			throw new NotSupportedException();
		}
		SettingsProperty property = (SettingsProperty)items[name];
		OnRemove(property);
		items.Remove(name);
		OnRemoveComplete(property);
	}

	public void SetReadOnly()
	{
		isReadOnly = true;
	}

	protected virtual void OnAdd(SettingsProperty property)
	{
	}

	protected virtual void OnAddComplete(SettingsProperty property)
	{
	}

	protected virtual void OnClear()
	{
	}

	protected virtual void OnClearComplete()
	{
	}

	protected virtual void OnRemove(SettingsProperty property)
	{
	}

	protected virtual void OnRemoveComplete(SettingsProperty property)
	{
	}
}

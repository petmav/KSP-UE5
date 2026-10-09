using System.Collections;

namespace System.Configuration;

public class SettingsPropertyValueCollection : ICloneable, ICollection, IEnumerable
{
	private Hashtable items;

	private bool isReadOnly;

	public int Count => items.Count;

	public bool IsSynchronized
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public SettingsPropertyValue this[string name] => (SettingsPropertyValue)items[name];

	public object SyncRoot
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public SettingsPropertyValueCollection()
	{
		items = new Hashtable();
	}

	public void Add(SettingsPropertyValue property)
	{
		if (isReadOnly)
		{
			throw new NotSupportedException();
		}
		items.Add(property.Name, property);
	}

	internal void Add(SettingsPropertyValueCollection vals)
	{
		foreach (SettingsPropertyValue val in vals)
		{
			Add(val);
		}
	}

	public void Clear()
	{
		if (isReadOnly)
		{
			throw new NotSupportedException();
		}
		items.Clear();
	}

	public object Clone()
	{
		return new SettingsPropertyValueCollection
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
		items.Remove(name);
	}

	public void SetReadOnly()
	{
		isReadOnly = true;
	}
}

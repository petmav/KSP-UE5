using System.Collections;
using System.Collections.Generic;

namespace System.Configuration;

public class ConfigurationPropertyCollection : ICollection, IEnumerable
{
	private List<ConfigurationProperty> collection;

	public int Count => collection.Count;

	public ConfigurationProperty this[string name]
	{
		get
		{
			foreach (ConfigurationProperty item in collection)
			{
				if (item.Name == name)
				{
					return item;
				}
			}
			return null;
		}
	}

	public bool IsSynchronized => false;

	public object SyncRoot => collection;

	public ConfigurationPropertyCollection()
	{
		collection = new List<ConfigurationProperty>();
	}

	public void Add(ConfigurationProperty property)
	{
		if (property == null)
		{
			throw new ArgumentNullException("property");
		}
		collection.Add(property);
	}

	public bool Contains(string name)
	{
		ConfigurationProperty configurationProperty = this[name];
		if (configurationProperty == null)
		{
			return false;
		}
		return collection.Contains(configurationProperty);
	}

	public void CopyTo(ConfigurationProperty[] array, int index)
	{
		collection.CopyTo(array, index);
	}

	void ICollection.CopyTo(Array array, int index)
	{
		((ICollection)collection).CopyTo(array, index);
	}

	public IEnumerator GetEnumerator()
	{
		return collection.GetEnumerator();
	}

	public bool Remove(string name)
	{
		return collection.Remove(this[name]);
	}

	public void Clear()
	{
		collection.Clear();
	}
}

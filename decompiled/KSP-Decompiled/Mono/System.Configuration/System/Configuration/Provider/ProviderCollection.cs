using System.Collections;

namespace System.Configuration.Provider;

public class ProviderCollection : ICollection, IEnumerable
{
	private Hashtable lookup;

	private bool readOnly;

	private ArrayList values;

	public int Count => values.Count;

	public bool IsSynchronized => false;

	public object SyncRoot => this;

	public ProviderBase this[string name]
	{
		get
		{
			object obj = lookup[name];
			if (obj == null)
			{
				return null;
			}
			return values[(int)obj] as ProviderBase;
		}
	}

	public ProviderCollection()
	{
		lookup = new Hashtable(10, StringComparer.InvariantCultureIgnoreCase);
		values = new ArrayList();
	}

	public virtual void Add(ProviderBase provider)
	{
		if (readOnly)
		{
			throw new NotSupportedException();
		}
		if (provider == null || provider.Name == null)
		{
			throw new ArgumentNullException();
		}
		int num = values.Add(provider);
		try
		{
			lookup.Add(provider.Name, num);
		}
		catch
		{
			values.RemoveAt(num);
			throw;
		}
	}

	public void Clear()
	{
		if (readOnly)
		{
			throw new NotSupportedException();
		}
		values.Clear();
		lookup.Clear();
	}

	public void CopyTo(ProviderBase[] array, int index)
	{
		values.CopyTo(array, index);
	}

	void ICollection.CopyTo(Array array, int index)
	{
		values.CopyTo(array, index);
	}

	public IEnumerator GetEnumerator()
	{
		return values.GetEnumerator();
	}

	public void Remove(string name)
	{
		if (readOnly)
		{
			throw new NotSupportedException();
		}
		object obj = lookup[name];
		if (obj == null || !(obj is int num))
		{
			throw new ArgumentException();
		}
		if (num >= values.Count)
		{
			throw new ArgumentException();
		}
		values.RemoveAt(num);
		lookup.Remove(name);
		ArrayList arrayList = new ArrayList();
		foreach (DictionaryEntry item in lookup)
		{
			if ((int)item.Value > num)
			{
				arrayList.Add(item.Key);
			}
		}
		foreach (string item2 in arrayList)
		{
			lookup[item2] = (int)lookup[item2] - 1;
		}
	}

	public void SetReadOnly()
	{
		readOnly = true;
	}
}

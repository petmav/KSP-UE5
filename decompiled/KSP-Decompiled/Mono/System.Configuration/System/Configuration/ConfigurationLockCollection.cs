using System.Collections;
using Unity;

namespace System.Configuration;

public sealed class ConfigurationLockCollection : ICollection, IEnumerable
{
	private ArrayList names;

	private ConfigurationElement element;

	private ConfigurationLockType lockType;

	private bool is_modified;

	private Hashtable valid_name_hash;

	private string valid_names;

	public string AttributeList
	{
		get
		{
			string[] array = new string[names.Count];
			names.CopyTo(array, 0);
			return string.Join(",", array);
		}
	}

	public int Count => names.Count;

	[System.MonoTODO]
	public bool HasParentElements => false;

	[System.MonoTODO]
	public bool IsModified
	{
		get
		{
			return is_modified;
		}
		internal set
		{
			is_modified = value;
		}
	}

	[System.MonoTODO]
	public bool IsSynchronized => false;

	[System.MonoTODO]
	public object SyncRoot => this;

	internal ConfigurationLockCollection(ConfigurationElement element, ConfigurationLockType lockType)
	{
		names = new ArrayList();
		this.element = element;
		this.lockType = lockType;
	}

	private void CheckName(string name)
	{
		bool flag = (lockType & ConfigurationLockType.Attribute) == ConfigurationLockType.Attribute;
		if (valid_name_hash == null)
		{
			valid_name_hash = new Hashtable();
			foreach (ConfigurationProperty property in element.Properties)
			{
				if (flag != property.IsElement)
				{
					valid_name_hash.Add(property.Name, true);
				}
			}
			if (!flag)
			{
				ConfigurationElementCollection defaultCollection = element.GetDefaultCollection();
				valid_name_hash.Add(defaultCollection.AddElementName, true);
				valid_name_hash.Add(defaultCollection.ClearElementName, true);
				valid_name_hash.Add(defaultCollection.RemoveElementName, true);
			}
			string[] array = new string[valid_name_hash.Keys.Count];
			valid_name_hash.Keys.CopyTo(array, 0);
			valid_names = string.Join(",", array);
		}
		if (valid_name_hash[name] == null)
		{
			throw new ConfigurationErrorsException(string.Format("The {2} '{0}' is not valid in the locked list for this section.  The following {3} can be locked: '{1}'", name, valid_names, flag ? "attribute" : "element", flag ? "attributes" : "elements"));
		}
	}

	public void Add(string name)
	{
		CheckName(name);
		if (!names.Contains(name))
		{
			names.Add(name);
			is_modified = true;
		}
	}

	public void Clear()
	{
		names.Clear();
		is_modified = true;
	}

	public bool Contains(string name)
	{
		return names.Contains(name);
	}

	public void CopyTo(string[] array, int index)
	{
		names.CopyTo(array, index);
	}

	public IEnumerator GetEnumerator()
	{
		return names.GetEnumerator();
	}

	[System.MonoInternalNote("we can't possibly *always* return false here...")]
	public bool IsReadOnly(string name)
	{
		for (int i = 0; i < names.Count; i++)
		{
			if ((string)names[i] == name)
			{
				return false;
			}
		}
		throw new ConfigurationErrorsException($"The entry '{name}' is not in the collection.");
	}

	public void Remove(string name)
	{
		names.Remove(name);
		is_modified = true;
	}

	public void SetFromList(string attributeList)
	{
		Clear();
		char[] separator = new char[1] { ',' };
		string[] array = attributeList.Split(separator);
		foreach (string text in array)
		{
			Add(text.Trim());
		}
	}

	void ICollection.CopyTo(Array array, int index)
	{
		names.CopyTo(array, index);
	}

	internal ConfigurationLockCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

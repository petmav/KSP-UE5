using System.Configuration;

namespace System.Net.Configuration;

[ConfigurationCollection(typeof(ConnectionManagementElement), CollectionType = ConfigurationElementCollectionType.AddRemoveClearMap)]
public sealed class ConnectionManagementElementCollection : ConfigurationElementCollection
{
	[System.MonoTODO]
	public ConnectionManagementElement this[int index]
	{
		get
		{
			throw new NotImplementedException();
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	public new ConnectionManagementElement this[string name]
	{
		get
		{
			return (ConnectionManagementElement)base[name];
		}
		set
		{
			base[name] = value;
		}
	}

	public void Add(ConnectionManagementElement element)
	{
		BaseAdd(element);
	}

	public void Clear()
	{
		BaseClear();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new ConnectionManagementElement();
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		if (!(element is ConnectionManagementElement))
		{
			throw new ArgumentException("element");
		}
		return ((ConnectionManagementElement)element).Address;
	}

	public int IndexOf(ConnectionManagementElement element)
	{
		return BaseIndexOf(element);
	}

	public void Remove(ConnectionManagementElement element)
	{
		BaseRemove(element);
	}

	public void Remove(string name)
	{
		BaseRemove(name);
	}

	public void RemoveAt(int index)
	{
		BaseRemoveAt(index);
	}
}

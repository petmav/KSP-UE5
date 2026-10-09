using System.Configuration;

namespace System.Net.Configuration;

[ConfigurationCollection(typeof(BypassElement), CollectionType = ConfigurationElementCollectionType.AddRemoveClearMap)]
public sealed class BypassElementCollection : ConfigurationElementCollection
{
	[System.MonoTODO]
	public BypassElement this[int index]
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

	public new BypassElement this[string name]
	{
		get
		{
			return (BypassElement)base[name];
		}
		set
		{
			base[name] = value;
		}
	}

	protected override bool ThrowOnDuplicate => false;

	public void Add(BypassElement element)
	{
		BaseAdd(element);
	}

	public void Clear()
	{
		BaseClear();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new BypassElement();
	}

	[System.MonoTODO("argument exception?")]
	protected override object GetElementKey(ConfigurationElement element)
	{
		if (!(element is BypassElement))
		{
			throw new ArgumentException("element");
		}
		return ((BypassElement)element).Address;
	}

	public int IndexOf(BypassElement element)
	{
		return BaseIndexOf(element);
	}

	public void Remove(BypassElement element)
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

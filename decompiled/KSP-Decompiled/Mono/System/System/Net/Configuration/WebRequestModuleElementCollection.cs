using System.Configuration;

namespace System.Net.Configuration;

[ConfigurationCollection(typeof(WebRequestModuleElement), CollectionType = ConfigurationElementCollectionType.AddRemoveClearMap)]
public sealed class WebRequestModuleElementCollection : ConfigurationElementCollection
{
	[System.MonoTODO]
	public WebRequestModuleElement this[int index]
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

	[System.MonoTODO]
	public new WebRequestModuleElement this[string name]
	{
		get
		{
			return (WebRequestModuleElement)base[name];
		}
		set
		{
			base[name] = value;
		}
	}

	public void Add(WebRequestModuleElement element)
	{
		BaseAdd(element);
	}

	public void Clear()
	{
		BaseClear();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new WebRequestModuleElement();
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		if (!(element is WebRequestModuleElement))
		{
			throw new ArgumentException("element");
		}
		return ((WebRequestModuleElement)element).Prefix;
	}

	public int IndexOf(WebRequestModuleElement element)
	{
		return BaseIndexOf(element);
	}

	public void Remove(WebRequestModuleElement element)
	{
		BaseRemove(element.Prefix);
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

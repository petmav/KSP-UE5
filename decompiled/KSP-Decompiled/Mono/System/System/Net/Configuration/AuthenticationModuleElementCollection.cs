using System.Configuration;

namespace System.Net.Configuration;

[ConfigurationCollection(typeof(AuthenticationModuleElement), CollectionType = ConfigurationElementCollectionType.AddRemoveClearMap)]
public sealed class AuthenticationModuleElementCollection : ConfigurationElementCollection
{
	[System.MonoTODO]
	public AuthenticationModuleElement this[int index]
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
	public new AuthenticationModuleElement this[string name]
	{
		get
		{
			return (AuthenticationModuleElement)base[name];
		}
		set
		{
			base[name] = value;
		}
	}

	[System.MonoTODO]
	public AuthenticationModuleElementCollection()
	{
	}

	public void Add(AuthenticationModuleElement element)
	{
		BaseAdd(element);
	}

	public void Clear()
	{
		BaseClear();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new AuthenticationModuleElement();
	}

	[System.MonoTODO("argument exception?")]
	protected override object GetElementKey(ConfigurationElement element)
	{
		if (!(element is AuthenticationModuleElement))
		{
			throw new ArgumentException("element");
		}
		return ((AuthenticationModuleElement)element).Type;
	}

	public int IndexOf(AuthenticationModuleElement element)
	{
		return BaseIndexOf(element);
	}

	public void Remove(AuthenticationModuleElement element)
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

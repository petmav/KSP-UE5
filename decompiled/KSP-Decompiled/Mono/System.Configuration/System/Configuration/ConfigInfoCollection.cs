using System.Collections;
using System.Collections.Specialized;

namespace System.Configuration;

internal class ConfigInfoCollection : NameObjectCollectionBase
{
	public ICollection AllKeys => Keys;

	public ConfigInfo this[string name]
	{
		get
		{
			return (ConfigInfo)BaseGet(name);
		}
		set
		{
			BaseSet(name, value);
		}
	}

	public ConfigInfo this[int index]
	{
		get
		{
			return (ConfigInfo)BaseGet(index);
		}
		set
		{
			BaseSet(index, value);
		}
	}

	public ConfigInfoCollection()
		: base(StringComparer.Ordinal)
	{
	}

	public void Add(string name, ConfigInfo config)
	{
		BaseAdd(name, config);
	}

	public void Clear()
	{
		BaseClear();
	}

	public string GetKey(int index)
	{
		return BaseGetKey(index);
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

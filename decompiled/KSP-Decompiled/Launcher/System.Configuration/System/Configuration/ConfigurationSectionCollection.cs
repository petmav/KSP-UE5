using System.Collections.Specialized;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Configuration;

[Serializable]
public sealed class ConfigurationSectionCollection : NameObjectCollectionBase
{
	public ConfigurationSection this[string name]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal ConfigurationSectionCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public ConfigurationSection get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Add(string name, ConfigurationSection section)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyTo(ConfigurationSection[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationSection Get(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public ConfigurationSection Get(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string GetKey(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Remove(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RemoveAt(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

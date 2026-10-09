using System.Collections.Specialized;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Configuration;

[Serializable]
public sealed class ConfigurationSectionGroupCollection : NameObjectCollectionBase
{
	public ConfigurationSectionGroup this[string name]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal ConfigurationSectionGroupCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public ConfigurationSectionGroup get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Add(string name, ConfigurationSectionGroup sectionGroup)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyTo(ConfigurationSectionGroup[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationSectionGroup Get(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public ConfigurationSectionGroup Get(string name)
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

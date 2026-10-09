using Unity;

namespace System.Configuration;

public sealed class SettingElementCollection : ConfigurationElementCollection
{
	public override ConfigurationElementCollectionType CollectionType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ConfigurationElementCollectionType);
		}
	}

	protected override string ElementName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingElementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(SettingElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public SettingElement Get(string elementKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Remove(SettingElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

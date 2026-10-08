using System.Runtime.CompilerServices;
using Unity;

namespace System.Configuration;

[ConfigurationCollection(typeof(SchemeSettingElement), CollectionType = ConfigurationElementCollectionType.AddRemoveClearMap, AddItemName = "add", ClearItemsName = "clear", RemoveItemName = "remove")]
public sealed class SchemeSettingElementCollection : ConfigurationElementCollection
{
	public override ConfigurationElementCollectionType CollectionType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ConfigurationElementCollectionType);
		}
	}

	public new SchemeSettingElement this[string name]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SchemeSettingElementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public SchemeSettingElement get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override ConfigurationElement CreateNewElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public int IndexOf(SchemeSettingElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}
}

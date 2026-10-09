namespace System.Configuration;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Property)]
public sealed class ConfigurationCollectionAttribute : Attribute
{
	private string addItemName = "add";

	private string clearItemsName = "clear";

	private string removeItemName = "remove";

	private ConfigurationElementCollectionType collectionType;

	private Type itemType;

	public string AddItemName
	{
		get
		{
			return addItemName;
		}
		set
		{
			addItemName = value;
		}
	}

	public string ClearItemsName
	{
		get
		{
			return clearItemsName;
		}
		set
		{
			clearItemsName = value;
		}
	}

	public string RemoveItemName
	{
		get
		{
			return removeItemName;
		}
		set
		{
			removeItemName = value;
		}
	}

	public ConfigurationElementCollectionType CollectionType
	{
		get
		{
			return collectionType;
		}
		set
		{
			collectionType = value;
		}
	}

	[System.MonoInternalNote("Do something with this in ConfigurationElementCollection")]
	public Type ItemType => itemType;

	public ConfigurationCollectionAttribute(Type itemType)
	{
		this.itemType = itemType;
	}
}

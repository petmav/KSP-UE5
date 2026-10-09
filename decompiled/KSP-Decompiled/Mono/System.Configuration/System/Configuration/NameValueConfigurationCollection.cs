namespace System.Configuration;

[ConfigurationCollection(typeof(NameValueConfigurationElement), AddItemName = "add", RemoveItemName = "remove", ClearItemsName = "clear", CollectionType = ConfigurationElementCollectionType.AddRemoveClearMap)]
public sealed class NameValueConfigurationCollection : ConfigurationElementCollection
{
	private static ConfigurationPropertyCollection properties;

	public string[] AllKeys => (string[])BaseGetAllKeys();

	public new NameValueConfigurationElement this[string name]
	{
		get
		{
			return (NameValueConfigurationElement)BaseGet(name);
		}
		set
		{
			throw new NotImplementedException();
		}
	}

	protected internal override ConfigurationPropertyCollection Properties => properties;

	static NameValueConfigurationCollection()
	{
		properties = new ConfigurationPropertyCollection();
	}

	public void Add(NameValueConfigurationElement nameValue)
	{
		BaseAdd(nameValue, throwIfExists: false);
	}

	public void Clear()
	{
		BaseClear();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new NameValueConfigurationElement("", "");
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		return ((NameValueConfigurationElement)element).Name;
	}

	public void Remove(NameValueConfigurationElement nameValue)
	{
		throw new NotImplementedException();
	}

	public void Remove(string name)
	{
		BaseRemove(name);
	}
}

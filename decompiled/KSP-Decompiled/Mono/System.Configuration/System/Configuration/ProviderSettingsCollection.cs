namespace System.Configuration;

[ConfigurationCollection(typeof(ProviderSettings), CollectionType = ConfigurationElementCollectionType.AddRemoveClearMap)]
public sealed class ProviderSettingsCollection : ConfigurationElementCollection
{
	private static ConfigurationPropertyCollection props = new ConfigurationPropertyCollection();

	public ProviderSettings this[int index]
	{
		get
		{
			return (ProviderSettings)BaseGet(index);
		}
		set
		{
			BaseAdd(index, value);
		}
	}

	public new ProviderSettings this[string key] => (ProviderSettings)BaseGet(key);

	protected internal override ConfigurationPropertyCollection Properties => props;

	public void Add(ProviderSettings provider)
	{
		BaseAdd(provider);
	}

	public void Clear()
	{
		BaseClear();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new ProviderSettings();
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		return ((ProviderSettings)element).Name;
	}

	public void Remove(string name)
	{
		BaseRemove(name);
	}
}

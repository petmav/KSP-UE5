using System.Collections;

namespace System.Configuration;

[ConfigurationCollection(typeof(KeyValueConfigurationElement), CollectionType = ConfigurationElementCollectionType.AddRemoveClearMap)]
public class KeyValueConfigurationCollection : ConfigurationElementCollection
{
	private ConfigurationPropertyCollection properties;

	public string[] AllKeys
	{
		get
		{
			string[] array = new string[base.Count];
			int num = 0;
			IEnumerator enumerator = GetEnumerator();
			try
			{
				while (enumerator.MoveNext())
				{
					KeyValueConfigurationElement keyValueConfigurationElement = (KeyValueConfigurationElement)enumerator.Current;
					array[num++] = keyValueConfigurationElement.Key;
				}
				return array;
			}
			finally
			{
				IDisposable disposable = enumerator as IDisposable;
				if (disposable != null)
				{
					disposable.Dispose();
				}
			}
		}
	}

	public new KeyValueConfigurationElement this[string key] => (KeyValueConfigurationElement)BaseGet(key);

	protected internal override ConfigurationPropertyCollection Properties
	{
		get
		{
			if (properties == null)
			{
				properties = new ConfigurationPropertyCollection();
			}
			return properties;
		}
	}

	protected override bool ThrowOnDuplicate => false;

	public void Add(KeyValueConfigurationElement keyValue)
	{
		keyValue.Init();
		BaseAdd(keyValue);
	}

	public void Add(string key, string value)
	{
		Add(new KeyValueConfigurationElement(key, value));
	}

	public void Clear()
	{
		BaseClear();
	}

	public void Remove(string key)
	{
		BaseRemove(key);
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new KeyValueConfigurationElement();
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		return ((KeyValueConfigurationElement)element).Key;
	}
}

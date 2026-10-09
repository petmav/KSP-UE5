using System.Collections;
using System.Globalization;

namespace System.Configuration;

[ConfigurationCollection(typeof(ConnectionStringSettings), CollectionType = ConfigurationElementCollectionType.AddRemoveClearMap)]
public sealed class ConnectionStringSettingsCollection : ConfigurationElementCollection
{
	public new ConnectionStringSettings this[string name]
	{
		get
		{
			IEnumerator enumerator = GetEnumerator();
			try
			{
				while (enumerator.MoveNext())
				{
					ConfigurationElement configurationElement = (ConfigurationElement)enumerator.Current;
					if (configurationElement is ConnectionStringSettings && string.Compare(((ConnectionStringSettings)configurationElement).Name, name, ignoreCase: true, CultureInfo.InvariantCulture) == 0)
					{
						return configurationElement as ConnectionStringSettings;
					}
				}
			}
			finally
			{
				IDisposable disposable = enumerator as IDisposable;
				if (disposable != null)
				{
					disposable.Dispose();
				}
			}
			return null;
		}
	}

	public ConnectionStringSettings this[int index]
	{
		get
		{
			return (ConnectionStringSettings)BaseGet(index);
		}
		set
		{
			if (BaseGet(index) != null)
			{
				BaseRemoveAt(index);
			}
			BaseAdd(index, value);
		}
	}

	[System.MonoTODO]
	protected internal override ConfigurationPropertyCollection Properties => base.Properties;

	protected override ConfigurationElement CreateNewElement()
	{
		return new ConnectionStringSettings();
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		return ((ConnectionStringSettings)element).Name;
	}

	public void Add(ConnectionStringSettings settings)
	{
		BaseAdd(settings);
	}

	public void Clear()
	{
		BaseClear();
	}

	public int IndexOf(ConnectionStringSettings settings)
	{
		return BaseIndexOf(settings);
	}

	public void Remove(ConnectionStringSettings settings)
	{
		BaseRemove(settings.Name);
	}

	public void Remove(string name)
	{
		BaseRemove(name);
	}

	public void RemoveAt(int index)
	{
		BaseRemoveAt(index);
	}

	protected override void BaseAdd(int index, ConfigurationElement element)
	{
		if (!(element is ConnectionStringSettings))
		{
			base.BaseAdd(element);
		}
		if (IndexOf((ConnectionStringSettings)element) >= 0)
		{
			throw new ConfigurationErrorsException($"The element {((ConnectionStringSettings)element).Name} already exist!");
		}
		this[index] = (ConnectionStringSettings)element;
	}
}

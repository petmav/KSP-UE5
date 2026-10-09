using System.Collections;

namespace System.Configuration;

public sealed class SettingElementCollection : ConfigurationElementCollection
{
	public override ConfigurationElementCollectionType CollectionType => ConfigurationElementCollectionType.BasicMap;

	protected override string ElementName => "setting";

	public void Add(SettingElement element)
	{
		BaseAdd(element);
	}

	public void Clear()
	{
		BaseClear();
	}

	public SettingElement Get(string elementKey)
	{
		IEnumerator enumerator = GetEnumerator();
		try
		{
			while (enumerator.MoveNext())
			{
				SettingElement settingElement = (SettingElement)enumerator.Current;
				if (settingElement.Name == elementKey)
				{
					return settingElement;
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

	public void Remove(SettingElement element)
	{
		if (element == null)
		{
			throw new ArgumentNullException("element");
		}
		BaseRemove(element.Name);
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new SettingElement();
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		return ((SettingElement)element).Name;
	}
}

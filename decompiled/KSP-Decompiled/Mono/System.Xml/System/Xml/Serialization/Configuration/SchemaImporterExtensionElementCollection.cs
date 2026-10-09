using System.Configuration;

namespace System.Xml.Serialization.Configuration;

[ConfigurationCollection(typeof(SchemaImporterExtensionElement))]
public sealed class SchemaImporterExtensionElementCollection : ConfigurationElementCollection
{
	public SchemaImporterExtensionElement this[int index]
	{
		get
		{
			return (SchemaImporterExtensionElement)BaseGet(index);
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

	public new SchemaImporterExtensionElement this[string name]
	{
		get
		{
			return (SchemaImporterExtensionElement)BaseGet(name);
		}
		set
		{
			if (BaseGet(name) != null)
			{
				BaseRemove(name);
			}
			BaseAdd(value);
		}
	}

	public void Add(SchemaImporterExtensionElement element)
	{
		BaseAdd(element);
	}

	public void Clear()
	{
		BaseClear();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new SchemaImporterExtensionElement();
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		return ((SchemaImporterExtensionElement)element).Key;
	}

	public int IndexOf(SchemaImporterExtensionElement element)
	{
		return BaseIndexOf(element);
	}

	public void Remove(SchemaImporterExtensionElement element)
	{
		BaseRemove(element.Key);
	}

	public void Remove(string name)
	{
		BaseRemove(name);
	}

	public void RemoveAt(int index)
	{
		BaseRemoveAt(index);
	}
}

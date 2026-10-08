using System.Collections;
using System.Diagnostics;
using System.Xml;

namespace System.Configuration;

[DebuggerDisplay("Count = {Count}")]
public abstract class ConfigurationElementCollection : ConfigurationElement, ICollection, IEnumerable
{
	private sealed class ConfigurationRemoveElement : ConfigurationElement
	{
		private readonly ConfigurationPropertyCollection properties = new ConfigurationPropertyCollection();

		private readonly ConfigurationElement _origElement;

		private readonly ConfigurationElementCollection _origCollection;

		internal object KeyValue
		{
			get
			{
				foreach (ConfigurationProperty property in Properties)
				{
					_origElement[property] = base[property];
				}
				return _origCollection.GetElementKey(_origElement);
			}
		}

		protected internal override ConfigurationPropertyCollection Properties => properties;

		internal ConfigurationRemoveElement(ConfigurationElement origElement, ConfigurationElementCollection origCollection)
		{
			_origElement = origElement;
			_origCollection = origCollection;
			foreach (ConfigurationProperty property in origElement.Properties)
			{
				if (property.IsKey)
				{
					properties.Add(property);
				}
			}
		}
	}

	private ArrayList list = new ArrayList();

	private ArrayList removed;

	private ArrayList inherited;

	private bool emitClear;

	private bool modified;

	private IComparer comparer;

	private int inheritedLimitIndex;

	private string addElementName = "add";

	private string clearElementName = "clear";

	private string removeElementName = "remove";

	public virtual ConfigurationElementCollectionType CollectionType => ConfigurationElementCollectionType.AddRemoveClearMap;

	private bool IsBasic
	{
		get
		{
			if (CollectionType != 0)
			{
				return CollectionType == ConfigurationElementCollectionType.BasicMapAlternate;
			}
			return true;
		}
	}

	private bool IsAlternate
	{
		get
		{
			if (CollectionType != ConfigurationElementCollectionType.AddRemoveClearMapAlternate)
			{
				return CollectionType == ConfigurationElementCollectionType.BasicMapAlternate;
			}
			return true;
		}
	}

	public int Count => list.Count;

	protected virtual string ElementName => string.Empty;

	public bool EmitClear
	{
		get
		{
			return emitClear;
		}
		set
		{
			emitClear = value;
		}
	}

	public bool IsSynchronized => false;

	public object SyncRoot => this;

	protected virtual bool ThrowOnDuplicate
	{
		get
		{
			if (CollectionType != ConfigurationElementCollectionType.AddRemoveClearMap && CollectionType != ConfigurationElementCollectionType.AddRemoveClearMapAlternate)
			{
				return false;
			}
			return true;
		}
	}

	protected internal string AddElementName
	{
		get
		{
			return addElementName;
		}
		set
		{
			addElementName = value;
		}
	}

	protected internal string ClearElementName
	{
		get
		{
			return clearElementName;
		}
		set
		{
			clearElementName = value;
		}
	}

	protected internal string RemoveElementName
	{
		get
		{
			return removeElementName;
		}
		set
		{
			removeElementName = value;
		}
	}

	protected ConfigurationElementCollection()
	{
	}

	protected ConfigurationElementCollection(IComparer comparer)
	{
		this.comparer = comparer;
	}

	internal override void InitFromProperty(PropertyInformation propertyInfo)
	{
		ConfigurationCollectionAttribute configurationCollectionAttribute = propertyInfo.Property.CollectionAttribute;
		if (configurationCollectionAttribute == null)
		{
			configurationCollectionAttribute = Attribute.GetCustomAttribute(propertyInfo.Type, typeof(ConfigurationCollectionAttribute)) as ConfigurationCollectionAttribute;
		}
		if (configurationCollectionAttribute != null)
		{
			addElementName = configurationCollectionAttribute.AddItemName;
			clearElementName = configurationCollectionAttribute.ClearItemsName;
			removeElementName = configurationCollectionAttribute.RemoveItemName;
		}
		base.InitFromProperty(propertyInfo);
	}

	protected virtual void BaseAdd(ConfigurationElement element)
	{
		BaseAdd(element, ThrowOnDuplicate);
	}

	protected void BaseAdd(ConfigurationElement element, bool throwIfExists)
	{
		if (IsReadOnly())
		{
			throw new ConfigurationErrorsException("Collection is read only.");
		}
		if (IsAlternate)
		{
			list.Insert(inheritedLimitIndex, element);
			inheritedLimitIndex++;
		}
		else
		{
			int num = IndexOfKey(GetElementKey(element));
			if (num >= 0)
			{
				if (element.Equals(list[num]))
				{
					return;
				}
				if (throwIfExists)
				{
					throw new ConfigurationErrorsException("Duplicate element in collection");
				}
				list.RemoveAt(num);
			}
			list.Add(element);
		}
		modified = true;
	}

	protected virtual void BaseAdd(int index, ConfigurationElement element)
	{
		if (ThrowOnDuplicate && BaseIndexOf(element) != -1)
		{
			throw new ConfigurationErrorsException("Duplicate element in collection");
		}
		if (IsReadOnly())
		{
			throw new ConfigurationErrorsException("Collection is read only.");
		}
		if (IsAlternate && index > inheritedLimitIndex)
		{
			throw new ConfigurationErrorsException("Can't insert new elements below the inherited elements.");
		}
		if (!IsAlternate && index <= inheritedLimitIndex)
		{
			throw new ConfigurationErrorsException("Can't insert new elements above the inherited elements.");
		}
		list.Insert(index, element);
		modified = true;
	}

	protected internal void BaseClear()
	{
		if (IsReadOnly())
		{
			throw new ConfigurationErrorsException("Collection is read only.");
		}
		list.Clear();
		modified = true;
	}

	protected internal ConfigurationElement BaseGet(int index)
	{
		return (ConfigurationElement)list[index];
	}

	protected internal ConfigurationElement BaseGet(object key)
	{
		int num = IndexOfKey(key);
		if (num != -1)
		{
			return (ConfigurationElement)list[num];
		}
		return null;
	}

	protected internal object[] BaseGetAllKeys()
	{
		object[] array = new object[list.Count];
		for (int i = 0; i < list.Count; i++)
		{
			array[i] = BaseGetKey(i);
		}
		return array;
	}

	protected internal object BaseGetKey(int index)
	{
		if (index < 0 || index >= list.Count)
		{
			throw new ConfigurationErrorsException($"Index {index} is out of range");
		}
		return GetElementKey((ConfigurationElement)list[index]).ToString();
	}

	protected int BaseIndexOf(ConfigurationElement element)
	{
		return list.IndexOf(element);
	}

	private int IndexOfKey(object key)
	{
		for (int i = 0; i < list.Count; i++)
		{
			if (CompareKeys(GetElementKey((ConfigurationElement)list[i]), key))
			{
				return i;
			}
		}
		return -1;
	}

	protected internal bool BaseIsRemoved(object key)
	{
		if (removed == null)
		{
			return false;
		}
		foreach (ConfigurationElement item in removed)
		{
			if (CompareKeys(GetElementKey(item), key))
			{
				return true;
			}
		}
		return false;
	}

	protected internal void BaseRemove(object key)
	{
		if (IsReadOnly())
		{
			throw new ConfigurationErrorsException("Collection is read only.");
		}
		int num = IndexOfKey(key);
		if (num != -1)
		{
			BaseRemoveAt(num);
			modified = true;
		}
	}

	protected internal void BaseRemoveAt(int index)
	{
		if (IsReadOnly())
		{
			throw new ConfigurationErrorsException("Collection is read only.");
		}
		ConfigurationElement configurationElement = (ConfigurationElement)list[index];
		if (!IsElementRemovable(configurationElement))
		{
			throw new ConfigurationErrorsException("Element can't be removed from element collection.");
		}
		if (inherited != null && inherited.Contains(configurationElement))
		{
			throw new ConfigurationErrorsException("Inherited items can't be removed.");
		}
		list.RemoveAt(index);
		if (IsAlternate && inheritedLimitIndex > 0)
		{
			inheritedLimitIndex--;
		}
		modified = true;
	}

	private bool CompareKeys(object key1, object key2)
	{
		if (comparer != null)
		{
			return comparer.Compare(key1, key2) == 0;
		}
		return object.Equals(key1, key2);
	}

	public void CopyTo(ConfigurationElement[] array, int index)
	{
		list.CopyTo(array, index);
	}

	protected abstract ConfigurationElement CreateNewElement();

	protected virtual ConfigurationElement CreateNewElement(string elementName)
	{
		return CreateNewElement();
	}

	private ConfigurationElement CreateNewElementInternal(string elementName)
	{
		ConfigurationElement configurationElement = ((elementName != null) ? CreateNewElement(elementName) : CreateNewElement());
		configurationElement.Init();
		return configurationElement;
	}

	public override bool Equals(object compareTo)
	{
		if (!(compareTo is ConfigurationElementCollection configurationElementCollection))
		{
			return false;
		}
		if (GetType() != configurationElementCollection.GetType())
		{
			return false;
		}
		if (Count != configurationElementCollection.Count)
		{
			return false;
		}
		for (int i = 0; i < Count; i++)
		{
			if (!BaseGet(i).Equals(configurationElementCollection.BaseGet(i)))
			{
				return false;
			}
		}
		return true;
	}

	protected abstract object GetElementKey(ConfigurationElement element);

	public override int GetHashCode()
	{
		int num = 0;
		for (int i = 0; i < Count; i++)
		{
			num += BaseGet(i).GetHashCode();
		}
		return num;
	}

	void ICollection.CopyTo(Array arr, int index)
	{
		list.CopyTo(arr, index);
	}

	public IEnumerator GetEnumerator()
	{
		return list.GetEnumerator();
	}

	protected virtual bool IsElementName(string elementName)
	{
		return false;
	}

	protected virtual bool IsElementRemovable(ConfigurationElement element)
	{
		return !IsReadOnly();
	}

	protected internal override bool IsModified()
	{
		if (modified)
		{
			return true;
		}
		for (int i = 0; i < list.Count; i++)
		{
			if (((ConfigurationElement)list[i]).IsModified())
			{
				modified = true;
				break;
			}
		}
		return modified;
	}

	[System.MonoTODO]
	public override bool IsReadOnly()
	{
		return base.IsReadOnly();
	}

	internal override void PrepareSave(ConfigurationElement parentElement, ConfigurationSaveMode mode)
	{
		ConfigurationElementCollection configurationElementCollection = (ConfigurationElementCollection)parentElement;
		base.PrepareSave(parentElement, mode);
		for (int i = 0; i < list.Count; i++)
		{
			ConfigurationElement configurationElement = (ConfigurationElement)list[i];
			object elementKey = GetElementKey(configurationElement);
			ConfigurationElement parent = configurationElementCollection?.BaseGet(elementKey);
			configurationElement.PrepareSave(parent, mode);
		}
	}

	internal override bool HasValues(ConfigurationElement parentElement, ConfigurationSaveMode mode)
	{
		ConfigurationElementCollection configurationElementCollection = (ConfigurationElementCollection)parentElement;
		if (mode == ConfigurationSaveMode.Full)
		{
			return list.Count > 0;
		}
		for (int i = 0; i < list.Count; i++)
		{
			ConfigurationElement configurationElement = (ConfigurationElement)list[i];
			object elementKey = GetElementKey(configurationElement);
			ConfigurationElement parent = configurationElementCollection?.BaseGet(elementKey);
			if (configurationElement.HasValues(parent, mode))
			{
				return true;
			}
		}
		return false;
	}

	protected internal override void Reset(ConfigurationElement parentElement)
	{
		bool isBasic = IsBasic;
		ConfigurationElementCollection configurationElementCollection = (ConfigurationElementCollection)parentElement;
		for (int i = 0; i < configurationElementCollection.Count; i++)
		{
			ConfigurationElement parentElement2 = configurationElementCollection.BaseGet(i);
			ConfigurationElement configurationElement = CreateNewElementInternal(null);
			configurationElement.Reset(parentElement2);
			BaseAdd(configurationElement);
			if (isBasic)
			{
				if (inherited == null)
				{
					inherited = new ArrayList();
				}
				inherited.Add(configurationElement);
			}
		}
		if (IsAlternate)
		{
			inheritedLimitIndex = 0;
		}
		else
		{
			inheritedLimitIndex = Count - 1;
		}
		modified = false;
	}

	protected internal override void ResetModified()
	{
		modified = false;
		for (int i = 0; i < list.Count; i++)
		{
			((ConfigurationElement)list[i]).ResetModified();
		}
	}

	[System.MonoTODO]
	protected internal override void SetReadOnly()
	{
		base.SetReadOnly();
	}

	protected internal override bool SerializeElement(XmlWriter writer, bool serializeCollectionKey)
	{
		if (serializeCollectionKey)
		{
			return base.SerializeElement(writer, serializeCollectionKey);
		}
		bool flag = false;
		if (IsBasic)
		{
			for (int i = 0; i < list.Count; i++)
			{
				ConfigurationElement configurationElement = (ConfigurationElement)list[i];
				flag = ((!(ElementName != string.Empty)) ? (configurationElement.SerializeElement(writer, serializeCollectionKey: false) || flag) : (configurationElement.SerializeToXmlElement(writer, ElementName) || flag));
			}
		}
		else
		{
			if (emitClear)
			{
				writer.WriteElementString(clearElementName, "");
				flag = true;
			}
			if (removed != null)
			{
				for (int j = 0; j < removed.Count; j++)
				{
					writer.WriteStartElement(removeElementName);
					((ConfigurationElement)removed[j]).SerializeElement(writer, serializeCollectionKey: true);
					writer.WriteEndElement();
				}
				flag = flag || removed.Count > 0;
			}
			for (int k = 0; k < list.Count; k++)
			{
				((ConfigurationElement)list[k]).SerializeToXmlElement(writer, addElementName);
			}
			flag = flag || list.Count > 0;
		}
		return flag;
	}

	protected override bool OnDeserializeUnrecognizedElement(string elementName, XmlReader reader)
	{
		if (IsBasic)
		{
			ConfigurationElement configurationElement = null;
			if (elementName == ElementName)
			{
				configurationElement = CreateNewElementInternal(null);
			}
			if (IsElementName(elementName))
			{
				configurationElement = CreateNewElementInternal(elementName);
			}
			if (configurationElement != null)
			{
				configurationElement.DeserializeElement(reader, serializeCollectionKey: false);
				BaseAdd(configurationElement);
				modified = false;
				return true;
			}
		}
		else
		{
			if (elementName == clearElementName)
			{
				reader.MoveToContent();
				if (reader.MoveToNextAttribute())
				{
					throw new ConfigurationErrorsException("Unrecognized attribute '" + reader.LocalName + "'.");
				}
				reader.MoveToElement();
				reader.Skip();
				BaseClear();
				emitClear = true;
				modified = false;
				return true;
			}
			if (elementName == removeElementName)
			{
				ConfigurationRemoveElement configurationRemoveElement = new ConfigurationRemoveElement(CreateNewElementInternal(null), this);
				configurationRemoveElement.DeserializeElement(reader, serializeCollectionKey: true);
				BaseRemove(configurationRemoveElement.KeyValue);
				modified = false;
				return true;
			}
			if (elementName == addElementName)
			{
				ConfigurationElement configurationElement2 = CreateNewElementInternal(null);
				configurationElement2.DeserializeElement(reader, serializeCollectionKey: false);
				BaseAdd(configurationElement2);
				modified = false;
				return true;
			}
		}
		return false;
	}

	protected internal override void Unmerge(ConfigurationElement sourceElement, ConfigurationElement parentElement, ConfigurationSaveMode saveMode)
	{
		ConfigurationElementCollection configurationElementCollection = (ConfigurationElementCollection)sourceElement;
		ConfigurationElementCollection configurationElementCollection2 = (ConfigurationElementCollection)parentElement;
		for (int i = 0; i < configurationElementCollection.Count; i++)
		{
			ConfigurationElement configurationElement = configurationElementCollection.BaseGet(i);
			object elementKey = configurationElementCollection.GetElementKey(configurationElement);
			ConfigurationElement configurationElement2 = configurationElementCollection2?.BaseGet(elementKey);
			ConfigurationElement configurationElement3 = CreateNewElementInternal(null);
			if (configurationElement2 != null && saveMode != ConfigurationSaveMode.Full)
			{
				configurationElement3.Unmerge(configurationElement, configurationElement2, saveMode);
				if (configurationElement3.HasValues(configurationElement2, saveMode))
				{
					BaseAdd(configurationElement3);
				}
			}
			else
			{
				configurationElement3.Unmerge(configurationElement, null, ConfigurationSaveMode.Full);
				BaseAdd(configurationElement3);
			}
		}
		if (saveMode == ConfigurationSaveMode.Full)
		{
			EmitClear = true;
		}
		else
		{
			if (configurationElementCollection2 == null)
			{
				return;
			}
			for (int j = 0; j < configurationElementCollection2.Count; j++)
			{
				ConfigurationElement configurationElement4 = configurationElementCollection2.BaseGet(j);
				object elementKey2 = configurationElementCollection2.GetElementKey(configurationElement4);
				if (configurationElementCollection.IndexOfKey(elementKey2) == -1)
				{
					if (removed == null)
					{
						removed = new ArrayList();
					}
					removed.Add(configurationElement4);
				}
			}
		}
	}
}

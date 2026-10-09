using System.Collections;
using System.Xml;
using Unity;

namespace System.Configuration;

public abstract class ConfigurationElement
{
	private class SaveContext
	{
		public readonly ConfigurationElement Element;

		public readonly ConfigurationElement Parent;

		public readonly ConfigurationSaveMode Mode;

		public SaveContext(ConfigurationElement element, ConfigurationElement parent, ConfigurationSaveMode mode)
		{
			Element = element;
			Parent = parent;
			Mode = mode;
		}

		public bool HasValues()
		{
			if (Mode == ConfigurationSaveMode.Full)
			{
				return true;
			}
			return Element.HasValues(Parent, Mode);
		}

		public bool HasValue(PropertyInformation prop)
		{
			if (Mode == ConfigurationSaveMode.Full)
			{
				return true;
			}
			return Element.HasValue(Parent, prop, Mode);
		}
	}

	private string rawXml;

	private bool modified;

	private ElementMap map;

	private ConfigurationPropertyCollection keyProps;

	private ConfigurationElementCollection defaultCollection;

	private bool readOnly;

	private ElementInformation elementInfo;

	private ConfigurationElementProperty elementProperty;

	private Configuration _configuration;

	private bool elementPresent;

	private ConfigurationLockCollection lockAllAttributesExcept;

	private ConfigurationLockCollection lockAllElementsExcept;

	private ConfigurationLockCollection lockAttributes;

	private ConfigurationLockCollection lockElements;

	private bool lockItem;

	private SaveContext saveContext;

	internal Configuration Configuration
	{
		get
		{
			return _configuration;
		}
		set
		{
			_configuration = value;
		}
	}

	public ElementInformation ElementInformation
	{
		get
		{
			if (elementInfo == null)
			{
				elementInfo = new ElementInformation(this, null);
			}
			return elementInfo;
		}
	}

	internal string RawXml
	{
		get
		{
			return rawXml;
		}
		set
		{
			if (rawXml == null || value != null)
			{
				rawXml = value;
			}
		}
	}

	protected internal virtual ConfigurationElementProperty ElementProperty
	{
		get
		{
			if (elementProperty == null)
			{
				elementProperty = new ConfigurationElementProperty(ElementInformation.Validator);
			}
			return elementProperty;
		}
	}

	protected ContextInformation EvaluationContext
	{
		get
		{
			if (Configuration != null)
			{
				return Configuration.EvaluationContext;
			}
			throw new ConfigurationErrorsException("This element is not currently associated with any context.");
		}
	}

	public ConfigurationLockCollection LockAllAttributesExcept
	{
		get
		{
			if (lockAllAttributesExcept == null)
			{
				lockAllAttributesExcept = new ConfigurationLockCollection(this, ConfigurationLockType.Attribute | ConfigurationLockType.Exclude);
			}
			return lockAllAttributesExcept;
		}
	}

	public ConfigurationLockCollection LockAllElementsExcept
	{
		get
		{
			if (lockAllElementsExcept == null)
			{
				lockAllElementsExcept = new ConfigurationLockCollection(this, ConfigurationLockType.Element | ConfigurationLockType.Exclude);
			}
			return lockAllElementsExcept;
		}
	}

	public ConfigurationLockCollection LockAttributes
	{
		get
		{
			if (lockAttributes == null)
			{
				lockAttributes = new ConfigurationLockCollection(this, ConfigurationLockType.Attribute);
			}
			return lockAttributes;
		}
	}

	public ConfigurationLockCollection LockElements
	{
		get
		{
			if (lockElements == null)
			{
				lockElements = new ConfigurationLockCollection(this, ConfigurationLockType.Element);
			}
			return lockElements;
		}
	}

	public bool LockItem
	{
		get
		{
			return lockItem;
		}
		set
		{
			lockItem = value;
		}
	}

	protected internal object this[ConfigurationProperty prop]
	{
		get
		{
			return this[prop.Name];
		}
		set
		{
			this[prop.Name] = value;
		}
	}

	protected internal object this[string propertyName]
	{
		get
		{
			return (ElementInformation.Properties[propertyName] ?? throw new InvalidOperationException("Property '" + propertyName + "' not found in configuration element")).Value;
		}
		set
		{
			PropertyInformation propertyInformation = ElementInformation.Properties[propertyName];
			if (propertyInformation == null)
			{
				throw new InvalidOperationException("Property '" + propertyName + "' not found in configuration element");
			}
			SetPropertyValue(propertyInformation.Property, value, ignoreLocks: false);
			propertyInformation.Value = value;
			modified = true;
		}
	}

	protected internal virtual ConfigurationPropertyCollection Properties
	{
		get
		{
			if (map == null)
			{
				map = ElementMap.GetMap(GetType());
			}
			return map.Properties;
		}
	}

	internal bool IsElementPresent => elementPresent;

	public Configuration CurrentConfiguration
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected bool HasContext
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	internal virtual void InitFromProperty(PropertyInformation propertyInfo)
	{
		elementInfo = new ElementInformation(this, propertyInfo);
		Init();
	}

	protected internal virtual void Init()
	{
	}

	[System.MonoTODO]
	protected virtual void ListErrors(IList errorList)
	{
		throw new NotImplementedException();
	}

	[System.MonoTODO]
	protected void SetPropertyValue(ConfigurationProperty prop, object value, bool ignoreLocks)
	{
		try
		{
			if (value != null)
			{
				prop.Validate(value);
			}
		}
		catch (Exception inner)
		{
			throw new ConfigurationErrorsException($"The value for the property '{prop.Name}' on type {ElementInformation.Type} is not valid.", inner);
		}
	}

	internal ConfigurationPropertyCollection GetKeyProperties()
	{
		if (keyProps != null)
		{
			return keyProps;
		}
		ConfigurationPropertyCollection configurationPropertyCollection = new ConfigurationPropertyCollection();
		foreach (ConfigurationProperty property in Properties)
		{
			if (property.IsKey)
			{
				configurationPropertyCollection.Add(property);
			}
		}
		return keyProps = configurationPropertyCollection;
	}

	internal ConfigurationElementCollection GetDefaultCollection()
	{
		if (defaultCollection != null)
		{
			return defaultCollection;
		}
		ConfigurationProperty configurationProperty = null;
		foreach (ConfigurationProperty property in Properties)
		{
			if (property.IsDefaultCollection)
			{
				configurationProperty = property;
				break;
			}
		}
		if (configurationProperty != null)
		{
			defaultCollection = this[configurationProperty] as ConfigurationElementCollection;
		}
		return defaultCollection;
	}

	public override bool Equals(object compareTo)
	{
		if (!(compareTo is ConfigurationElement configurationElement))
		{
			return false;
		}
		if (GetType() != configurationElement.GetType())
		{
			return false;
		}
		foreach (ConfigurationProperty property in Properties)
		{
			if (!object.Equals(this[property], configurationElement[property]))
			{
				return false;
			}
		}
		return true;
	}

	public override int GetHashCode()
	{
		int num = 0;
		foreach (ConfigurationProperty property in Properties)
		{
			object obj = this[property];
			if (obj != null)
			{
				num += obj.GetHashCode();
			}
		}
		return num;
	}

	internal virtual bool HasLocalModifications()
	{
		foreach (PropertyInformation property in ElementInformation.Properties)
		{
			if (property.ValueOrigin == PropertyValueOrigin.SetHere && property.IsModified)
			{
				return true;
			}
		}
		return false;
	}

	protected internal virtual void DeserializeElement(XmlReader reader, bool serializeCollectionKey)
	{
		Hashtable hashtable = new Hashtable();
		reader.MoveToContent();
		elementPresent = true;
		while (reader.MoveToNextAttribute())
		{
			PropertyInformation propertyInformation = ElementInformation.Properties[reader.LocalName];
			if (propertyInformation == null || (serializeCollectionKey && !propertyInformation.IsKey))
			{
				if (reader.LocalName == "lockAllAttributesExcept")
				{
					LockAllAttributesExcept.SetFromList(reader.Value);
				}
				else if (reader.LocalName == "lockAllElementsExcept")
				{
					LockAllElementsExcept.SetFromList(reader.Value);
				}
				else if (reader.LocalName == "lockAttributes")
				{
					LockAttributes.SetFromList(reader.Value);
				}
				else if (reader.LocalName == "lockElements")
				{
					LockElements.SetFromList(reader.Value);
				}
				else if (reader.LocalName == "lockItem")
				{
					LockItem = reader.Value.ToLowerInvariant() == "true";
				}
				else if (!(reader.LocalName == "xmlns") && (!(this is ConfigurationSection) || !(reader.LocalName == "configSource")) && !OnDeserializeUnrecognizedAttribute(reader.LocalName, reader.Value))
				{
					throw new ConfigurationErrorsException("Unrecognized attribute '" + reader.LocalName + "'.", reader);
				}
				continue;
			}
			if (hashtable.ContainsKey(propertyInformation))
			{
				throw new ConfigurationErrorsException("The attribute '" + propertyInformation.Name + "' may only appear once in this element.", reader);
			}
			string text = null;
			try
			{
				text = reader.Value;
				ValidateValue(propertyInformation.Property, text);
				propertyInformation.SetStringValue(text);
			}
			catch (ConfigurationErrorsException)
			{
				throw;
			}
			catch (ConfigurationException)
			{
				throw;
			}
			catch (Exception ex3)
			{
				throw new ConfigurationErrorsException($"The value for the property '{propertyInformation.Name}' is not valid. The error is: {ex3.Message}", reader);
			}
			hashtable[propertyInformation] = propertyInformation.Name;
			if (reader is ConfigXmlTextReader configXmlTextReader)
			{
				propertyInformation.Source = configXmlTextReader.Filename;
				propertyInformation.LineNumber = configXmlTextReader.LineNumber;
			}
		}
		reader.MoveToElement();
		if (reader.IsEmptyElement)
		{
			reader.Skip();
		}
		else
		{
			int depth = reader.Depth;
			reader.ReadStartElement();
			reader.MoveToContent();
			do
			{
				if (reader.NodeType != XmlNodeType.Element)
				{
					reader.Skip();
					continue;
				}
				PropertyInformation propertyInformation2 = ElementInformation.Properties[reader.LocalName];
				if (propertyInformation2 == null || (serializeCollectionKey && !propertyInformation2.IsKey))
				{
					if (OnDeserializeUnrecognizedElement(reader.LocalName, reader))
					{
						continue;
					}
					if (propertyInformation2 == null)
					{
						ConfigurationElementCollection configurationElementCollection = GetDefaultCollection();
						if (configurationElementCollection != null && configurationElementCollection.OnDeserializeUnrecognizedElement(reader.LocalName, reader))
						{
							continue;
						}
					}
					throw new ConfigurationErrorsException("Unrecognized element '" + reader.LocalName + "'.", reader);
				}
				if (!propertyInformation2.IsElement)
				{
					throw new ConfigurationErrorsException("Property '" + propertyInformation2.Name + "' is not a ConfigurationElement.");
				}
				if (hashtable.Contains(propertyInformation2))
				{
					throw new ConfigurationErrorsException("The element <" + propertyInformation2.Name + "> may only appear once in this section.", reader);
				}
				((ConfigurationElement)propertyInformation2.Value).DeserializeElement(reader, serializeCollectionKey);
				hashtable[propertyInformation2] = propertyInformation2.Name;
				if (depth == reader.Depth)
				{
					reader.Read();
				}
			}
			while (depth < reader.Depth);
		}
		modified = false;
		foreach (PropertyInformation property in ElementInformation.Properties)
		{
			if (!string.IsNullOrEmpty(property.Name) && property.IsRequired && !hashtable.ContainsKey(property) && ElementInformation.Properties[property.Name] == null)
			{
				object obj = OnRequiredPropertyNotFound(property.Name);
				if (!object.Equals(obj, property.DefaultValue))
				{
					property.Value = obj;
					property.IsModified = false;
				}
			}
		}
		PostDeserialize();
	}

	protected virtual bool OnDeserializeUnrecognizedAttribute(string name, string value)
	{
		return false;
	}

	protected virtual bool OnDeserializeUnrecognizedElement(string elementName, XmlReader reader)
	{
		return false;
	}

	protected virtual object OnRequiredPropertyNotFound(string name)
	{
		throw new ConfigurationErrorsException("Required attribute '" + name + "' not found.");
	}

	protected virtual void PreSerialize(XmlWriter writer)
	{
	}

	protected virtual void PostDeserialize()
	{
	}

	protected internal virtual void InitializeDefault()
	{
	}

	protected internal virtual bool IsModified()
	{
		if (modified)
		{
			return true;
		}
		foreach (PropertyInformation property in ElementInformation.Properties)
		{
			if (property.IsElement && property.Value is ConfigurationElement configurationElement && configurationElement.IsModified())
			{
				modified = true;
				break;
			}
		}
		return modified;
	}

	protected internal virtual void SetReadOnly()
	{
		readOnly = true;
	}

	public virtual bool IsReadOnly()
	{
		return readOnly;
	}

	protected internal virtual void Reset(ConfigurationElement parentElement)
	{
		elementPresent = false;
		if (parentElement != null)
		{
			ElementInformation.Reset(parentElement.ElementInformation);
		}
		else
		{
			InitializeDefault();
		}
	}

	protected internal virtual void ResetModified()
	{
		modified = false;
		foreach (PropertyInformation property in ElementInformation.Properties)
		{
			property.IsModified = false;
			if (property.Value is ConfigurationElement configurationElement)
			{
				configurationElement.ResetModified();
			}
		}
	}

	protected internal virtual bool SerializeElement(XmlWriter writer, bool serializeCollectionKey)
	{
		PreSerialize(writer);
		if (serializeCollectionKey)
		{
			ConfigurationPropertyCollection keyProperties = GetKeyProperties();
			foreach (ConfigurationProperty item in keyProperties)
			{
				writer.WriteAttributeString(item.Name, item.ConvertToString(this[item.Name]));
			}
			return keyProperties.Count > 0;
		}
		bool flag = false;
		foreach (PropertyInformation property in ElementInformation.Properties)
		{
			if (!property.IsElement)
			{
				if (saveContext == null)
				{
					throw new InvalidOperationException();
				}
				if (saveContext.HasValue(property))
				{
					writer.WriteAttributeString(property.Name, property.GetStringValue());
					flag = true;
				}
			}
		}
		foreach (PropertyInformation property2 in ElementInformation.Properties)
		{
			if (property2.IsElement)
			{
				ConfigurationElement configurationElement = (ConfigurationElement)property2.Value;
				if (configurationElement != null)
				{
					flag = configurationElement.SerializeToXmlElement(writer, property2.Name) || flag;
				}
			}
		}
		return flag;
	}

	protected internal virtual bool SerializeToXmlElement(XmlWriter writer, string elementName)
	{
		if (saveContext == null)
		{
			throw new InvalidOperationException();
		}
		if (!saveContext.HasValues())
		{
			return false;
		}
		if (elementName != null && elementName != "")
		{
			writer.WriteStartElement(elementName);
		}
		bool result = SerializeElement(writer, serializeCollectionKey: false);
		if (elementName != null && elementName != "")
		{
			writer.WriteEndElement();
		}
		return result;
	}

	protected internal virtual void Unmerge(ConfigurationElement sourceElement, ConfigurationElement parentElement, ConfigurationSaveMode saveMode)
	{
		if (parentElement != null && sourceElement.GetType() != parentElement.GetType())
		{
			throw new ConfigurationErrorsException("Can't unmerge two elements of different type");
		}
		bool flag = saveMode == ConfigurationSaveMode.Minimal || saveMode == ConfigurationSaveMode.Modified;
		foreach (PropertyInformation property in sourceElement.ElementInformation.Properties)
		{
			if (property.ValueOrigin == PropertyValueOrigin.Default)
			{
				continue;
			}
			PropertyInformation propertyInformation2 = ElementInformation.Properties[property.Name];
			object value = property.Value;
			if (parentElement == null || !parentElement.HasValue(property.Name))
			{
				propertyInformation2.Value = value;
			}
			else
			{
				if (value == null)
				{
					continue;
				}
				object obj = parentElement[property.Name];
				if (!property.IsElement)
				{
					if (!object.Equals(value, obj) || saveMode == ConfigurationSaveMode.Full || (saveMode == ConfigurationSaveMode.Modified && property.ValueOrigin == PropertyValueOrigin.SetHere))
					{
						propertyInformation2.Value = value;
					}
					continue;
				}
				ConfigurationElement configurationElement = (ConfigurationElement)value;
				if (!flag || configurationElement.IsModified())
				{
					if (obj == null)
					{
						propertyInformation2.Value = value;
						continue;
					}
					ConfigurationElement parentElement2 = (ConfigurationElement)obj;
					((ConfigurationElement)propertyInformation2.Value).Unmerge(configurationElement, parentElement2, saveMode);
				}
			}
		}
	}

	internal bool HasValue(string propName)
	{
		PropertyInformation propertyInformation = ElementInformation.Properties[propName];
		if (propertyInformation != null)
		{
			return propertyInformation.ValueOrigin != PropertyValueOrigin.Default;
		}
		return false;
	}

	internal bool IsReadFromConfig(string propName)
	{
		PropertyInformation propertyInformation = ElementInformation.Properties[propName];
		if (propertyInformation != null)
		{
			return propertyInformation.ValueOrigin == PropertyValueOrigin.SetHere;
		}
		return false;
	}

	private void ValidateValue(ConfigurationProperty p, string value)
	{
		ConfigurationValidatorBase validator;
		if (p != null && (validator = p.Validator) != null)
		{
			if (!validator.CanValidate(p.Type))
			{
				throw new ConfigurationErrorsException($"Validator does not support type {p.Type}");
			}
			validator.Validate(p.ConvertFromString(value));
		}
	}

	internal bool HasValue(ConfigurationElement parent, PropertyInformation prop, ConfigurationSaveMode mode)
	{
		if (prop.ValueOrigin == PropertyValueOrigin.Default)
		{
			return false;
		}
		if (mode == ConfigurationSaveMode.Modified && prop.ValueOrigin == PropertyValueOrigin.SetHere && prop.IsModified)
		{
			return true;
		}
		object obj = ((parent != null && parent.HasValue(prop.Name)) ? parent[prop.Name] : prop.DefaultValue);
		if (!prop.IsElement)
		{
			return !object.Equals(prop.Value, obj);
		}
		ConfigurationElement obj2 = (ConfigurationElement)prop.Value;
		ConfigurationElement parent2 = (ConfigurationElement)obj;
		return obj2.HasValues(parent2, mode);
	}

	internal virtual bool HasValues(ConfigurationElement parent, ConfigurationSaveMode mode)
	{
		if (mode == ConfigurationSaveMode.Full)
		{
			return true;
		}
		if (modified && mode == ConfigurationSaveMode.Modified)
		{
			return true;
		}
		foreach (PropertyInformation property in ElementInformation.Properties)
		{
			if (HasValue(parent, property, mode))
			{
				return true;
			}
		}
		return false;
	}

	internal virtual void PrepareSave(ConfigurationElement parent, ConfigurationSaveMode mode)
	{
		saveContext = new SaveContext(this, parent, mode);
		foreach (PropertyInformation property in ElementInformation.Properties)
		{
			if (property.IsElement)
			{
				ConfigurationElement configurationElement = (ConfigurationElement)property.Value;
				if (parent == null || !parent.HasValue(property.Name))
				{
					configurationElement.PrepareSave(null, mode);
					continue;
				}
				ConfigurationElement parent2 = (ConfigurationElement)parent[property.Name];
				configurationElement.PrepareSave(parent2, mode);
			}
		}
	}

	protected virtual string GetTransformedAssemblyString(string assemblyName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual string GetTransformedTypeString(string typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

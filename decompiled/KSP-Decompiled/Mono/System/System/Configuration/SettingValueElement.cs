using System.Reflection;
using System.Xml;

namespace System.Configuration;

public sealed class SettingValueElement : ConfigurationElement
{
	private XmlNode node;

	private XmlNode original;

	[System.MonoTODO]
	protected override ConfigurationPropertyCollection Properties => base.Properties;

	public XmlNode ValueXml
	{
		get
		{
			return node;
		}
		set
		{
			node = value;
		}
	}

	[System.MonoTODO]
	public SettingValueElement()
	{
	}

	[System.MonoTODO]
	protected override void DeserializeElement(XmlReader reader, bool serializeCollectionKey)
	{
		original = new XmlDocument().ReadNode(reader);
		node = original.CloneNode(deep: true);
	}

	public override bool Equals(object settingValue)
	{
		throw new NotImplementedException();
	}

	public override int GetHashCode()
	{
		throw new NotImplementedException();
	}

	protected override bool IsModified()
	{
		return original != node;
	}

	protected override void Reset(ConfigurationElement parentElement)
	{
		node = null;
	}

	protected override void ResetModified()
	{
		node = original;
	}

	protected override bool SerializeToXmlElement(XmlWriter writer, string elementName)
	{
		if (node == null)
		{
			return false;
		}
		node.WriteTo(writer);
		return true;
	}

	protected override void Unmerge(ConfigurationElement sourceElement, ConfigurationElement parentElement, ConfigurationSaveMode saveMode)
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
			PropertyInformation propertyInformation2 = base.ElementInformation.Properties[property.Name];
			object value = property.Value;
			if (parentElement == null || !HasValue(parentElement, property.Name))
			{
				propertyInformation2.Value = value;
			}
			else
			{
				if (value == null)
				{
					continue;
				}
				object item = GetItem(parentElement, property.Name);
				if (!PropertyIsElement(property))
				{
					if (!object.Equals(value, item) || saveMode == ConfigurationSaveMode.Full || (saveMode == ConfigurationSaveMode.Modified && property.ValueOrigin == PropertyValueOrigin.SetHere))
					{
						propertyInformation2.Value = value;
					}
					continue;
				}
				ConfigurationElement configurationElement = (ConfigurationElement)value;
				if (!flag || ElementIsModified(configurationElement))
				{
					if (item == null)
					{
						propertyInformation2.Value = value;
						continue;
					}
					ConfigurationElement parentElement2 = (ConfigurationElement)item;
					ConfigurationElement target = (ConfigurationElement)propertyInformation2.Value;
					ElementUnmerge(target, configurationElement, parentElement2, saveMode);
				}
			}
		}
	}

	private bool HasValue(ConfigurationElement element, string propName)
	{
		PropertyInformation propertyInformation = element.ElementInformation.Properties[propName];
		if (propertyInformation != null)
		{
			return propertyInformation.ValueOrigin != PropertyValueOrigin.Default;
		}
		return false;
	}

	private object GetItem(ConfigurationElement element, string property)
	{
		return (base.ElementInformation.Properties[property] ?? throw new InvalidOperationException("Property '" + property + "' not found in configuration element")).Value;
	}

	private bool PropertyIsElement(PropertyInformation prop)
	{
		return typeof(ConfigurationElement).IsAssignableFrom(prop.Type);
	}

	private bool ElementIsModified(ConfigurationElement element)
	{
		return (bool)element.GetType().GetMethod("IsModified", BindingFlags.Instance | BindingFlags.NonPublic).Invoke(element, new object[0]);
	}

	private void ElementUnmerge(ConfigurationElement target, ConfigurationElement sourceElement, ConfigurationElement parentElement, ConfigurationSaveMode saveMode)
	{
		target.GetType().GetMethod("Unmerge", BindingFlags.Instance | BindingFlags.NonPublic).Invoke(target, new object[3] { sourceElement, parentElement, saveMode });
	}
}

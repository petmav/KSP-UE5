using System.ComponentModel;
using System.Globalization;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Xml;
using System.Xml.Serialization;

namespace System.Configuration;

public class SettingsPropertyValue
{
	private readonly SettingsProperty property;

	private object propertyValue;

	private object serializedValue;

	private bool needSerializedValue;

	private bool needPropertyValue;

	private bool dirty;

	private bool defaulted;

	private bool deserialized;

	public bool Deserialized
	{
		get
		{
			return deserialized;
		}
		set
		{
			deserialized = value;
		}
	}

	public bool IsDirty
	{
		get
		{
			return dirty;
		}
		set
		{
			dirty = value;
		}
	}

	public string Name => property.Name;

	public SettingsProperty Property => property;

	public object PropertyValue
	{
		get
		{
			if (needPropertyValue)
			{
				propertyValue = GetDeserializedValue(serializedValue);
				if (propertyValue == null)
				{
					propertyValue = GetDeserializedDefaultValue();
					defaulted = true;
				}
				needPropertyValue = false;
			}
			if (propertyValue != null && !(propertyValue is string) && !(propertyValue is DateTime) && !property.PropertyType.IsPrimitive)
			{
				dirty = true;
			}
			return propertyValue;
		}
		set
		{
			propertyValue = value;
			dirty = true;
			needPropertyValue = false;
			needSerializedValue = true;
			defaulted = false;
		}
	}

	public object SerializedValue
	{
		get
		{
			if (needSerializedValue)
			{
				needSerializedValue = false;
				switch (property.SerializeAs)
				{
				case SettingsSerializeAs.String:
					serializedValue = TypeDescriptor.GetConverter(property.PropertyType).ConvertToInvariantString(propertyValue);
					break;
				case SettingsSerializeAs.Xml:
					if (propertyValue != null)
					{
						XmlSerializer xmlSerializer = new XmlSerializer(propertyValue.GetType());
						StringWriter stringWriter = new StringWriter(CultureInfo.InvariantCulture);
						xmlSerializer.Serialize(stringWriter, propertyValue);
						serializedValue = stringWriter.ToString();
					}
					else
					{
						serializedValue = null;
					}
					break;
				case SettingsSerializeAs.Binary:
					if (propertyValue != null)
					{
						BinaryFormatter binaryFormatter = new BinaryFormatter();
						MemoryStream memoryStream = new MemoryStream();
						binaryFormatter.Serialize(memoryStream, propertyValue);
						serializedValue = memoryStream.ToArray();
					}
					else
					{
						serializedValue = null;
					}
					break;
				default:
					serializedValue = null;
					break;
				}
			}
			return serializedValue;
		}
		set
		{
			serializedValue = value;
			needPropertyValue = true;
		}
	}

	public bool UsingDefaultValue => defaulted;

	public SettingsPropertyValue(SettingsProperty property)
	{
		this.property = property;
		needPropertyValue = true;
	}

	internal object Reset()
	{
		propertyValue = GetDeserializedDefaultValue();
		dirty = true;
		defaulted = true;
		needPropertyValue = true;
		return propertyValue;
	}

	private object GetDeserializedDefaultValue()
	{
		if (property.DefaultValue == null)
		{
			if (property.PropertyType != null && property.PropertyType.IsValueType)
			{
				return Activator.CreateInstance(property.PropertyType);
			}
			return null;
		}
		if (property.DefaultValue is string && ((string)property.DefaultValue).Length == 0)
		{
			if (property.PropertyType != typeof(string))
			{
				return Activator.CreateInstance(property.PropertyType);
			}
			return string.Empty;
		}
		if (property.DefaultValue is string && ((string)property.DefaultValue).Length > 0)
		{
			return GetDeserializedValue(property.DefaultValue);
		}
		if (!property.PropertyType.IsAssignableFrom(property.DefaultValue.GetType()))
		{
			return TypeDescriptor.GetConverter(property.PropertyType).ConvertFrom(null, CultureInfo.InvariantCulture, property.DefaultValue);
		}
		return property.DefaultValue;
	}

	private object GetDeserializedValue(object serializedValue)
	{
		if (serializedValue == null)
		{
			return null;
		}
		object result = null;
		try
		{
			switch (property.SerializeAs)
			{
			case SettingsSerializeAs.String:
				if (serializedValue is string)
				{
					result = TypeDescriptor.GetConverter(property.PropertyType).ConvertFromInvariantString((string)serializedValue);
				}
				break;
			case SettingsSerializeAs.Xml:
			{
				XmlSerializer xmlSerializer = new XmlSerializer(property.PropertyType);
				StringReader input = new StringReader((string)serializedValue);
				result = xmlSerializer.Deserialize(XmlReader.Create(input));
				break;
			}
			case SettingsSerializeAs.Binary:
			{
				BinaryFormatter binaryFormatter = new BinaryFormatter();
				MemoryStream serializationStream = ((!(serializedValue is string)) ? new MemoryStream((byte[])serializedValue) : new MemoryStream(Convert.FromBase64String((string)serializedValue)));
				result = binaryFormatter.Deserialize(serializationStream);
				break;
			}
			}
		}
		catch (Exception ex)
		{
			if (property.ThrowOnErrorDeserializing)
			{
				throw ex;
			}
		}
		return result;
	}
}

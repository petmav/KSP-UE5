using System.Collections;
using System.ComponentModel;
using System.Reflection;

namespace System.Configuration;

internal class ElementMap
{
	private static readonly Hashtable elementMaps = Hashtable.Synchronized(new Hashtable());

	private readonly ConfigurationPropertyCollection properties;

	private readonly ConfigurationCollectionAttribute collectionAttribute;

	public ConfigurationCollectionAttribute CollectionAttribute => collectionAttribute;

	public bool HasProperties => properties.Count > 0;

	public ConfigurationPropertyCollection Properties => properties;

	public static ElementMap GetMap(Type t)
	{
		if (elementMaps[t] is ElementMap result)
		{
			return result;
		}
		ElementMap elementMap = new ElementMap(t);
		elementMaps[t] = elementMap;
		return elementMap;
	}

	public ElementMap(Type t)
	{
		properties = new ConfigurationPropertyCollection();
		collectionAttribute = Attribute.GetCustomAttribute(t, typeof(ConfigurationCollectionAttribute)) as ConfigurationCollectionAttribute;
		PropertyInfo[] array = t.GetProperties(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
		foreach (PropertyInfo propertyInfo in array)
		{
			if (Attribute.GetCustomAttribute(propertyInfo, typeof(ConfigurationPropertyAttribute)) is ConfigurationPropertyAttribute configurationPropertyAttribute)
			{
				string name = ((configurationPropertyAttribute.Name != null) ? configurationPropertyAttribute.Name : propertyInfo.Name);
				ConfigurationValidatorBase validator = ((Attribute.GetCustomAttribute(propertyInfo, typeof(ConfigurationValidatorAttribute)) is ConfigurationValidatorAttribute configurationValidatorAttribute) ? configurationValidatorAttribute.ValidatorInstance : null);
				TypeConverterAttribute typeConverterAttribute = (TypeConverterAttribute)Attribute.GetCustomAttribute(propertyInfo, typeof(TypeConverterAttribute));
				ConfigurationProperty property = new ConfigurationProperty(typeConverter: (typeConverterAttribute != null) ? ((TypeConverter)Activator.CreateInstance(Type.GetType(typeConverterAttribute.ConverterTypeName), nonPublic: true)) : null, name: name, type: propertyInfo.PropertyType, defaultValue: configurationPropertyAttribute.DefaultValue, validator: validator, options: configurationPropertyAttribute.Options)
				{
					CollectionAttribute = (Attribute.GetCustomAttribute(propertyInfo, typeof(ConfigurationCollectionAttribute)) as ConfigurationCollectionAttribute)
				};
				properties.Add(property);
			}
		}
	}
}

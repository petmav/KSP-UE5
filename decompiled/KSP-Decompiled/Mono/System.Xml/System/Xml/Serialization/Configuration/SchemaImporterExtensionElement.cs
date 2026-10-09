using System.ComponentModel;
using System.Configuration;
using System.Globalization;

namespace System.Xml.Serialization.Configuration;

public sealed class SchemaImporterExtensionElement : ConfigurationElement
{
	private class TypeAndName
	{
		public readonly Type type;

		public readonly string name;

		public TypeAndName(string name)
		{
			type = Type.GetType(name, throwOnError: true, ignoreCase: true);
			this.name = name;
		}

		public TypeAndName(Type type)
		{
			this.type = type;
		}

		public override int GetHashCode()
		{
			return type.GetHashCode();
		}

		public override bool Equals(object comparand)
		{
			return type.Equals(((TypeAndName)comparand).type);
		}
	}

	private class TypeTypeConverter : TypeConverter
	{
		public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
		{
			if (sourceType == typeof(string))
			{
				return true;
			}
			return base.CanConvertFrom(context, sourceType);
		}

		public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
		{
			if (value is string)
			{
				return new TypeAndName((string)value);
			}
			return base.ConvertFrom(context, culture, value);
		}

		public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
		{
			if (destinationType == typeof(string))
			{
				TypeAndName typeAndName = (TypeAndName)value;
				if (typeAndName.name != null)
				{
					return typeAndName.name;
				}
				return typeAndName.type.AssemblyQualifiedName;
			}
			return base.ConvertTo(context, culture, value, destinationType);
		}
	}

	private ConfigurationPropertyCollection properties = new ConfigurationPropertyCollection();

	private readonly ConfigurationProperty name = new ConfigurationProperty("name", typeof(string), null, ConfigurationPropertyOptions.IsKey);

	private readonly ConfigurationProperty type = new ConfigurationProperty("type", typeof(Type), null, new TypeTypeConverter(), null, ConfigurationPropertyOptions.IsRequired);

	[ConfigurationProperty("name", IsRequired = true, IsKey = true)]
	public string Name
	{
		get
		{
			return (string)base[name];
		}
		set
		{
			base[name] = value;
		}
	}

	protected override ConfigurationPropertyCollection Properties => properties;

	[TypeConverter(typeof(TypeTypeConverter))]
	[ConfigurationProperty("type", IsRequired = true, IsKey = false)]
	public Type Type
	{
		get
		{
			return ((TypeAndName)base[type]).type;
		}
		set
		{
			base[type] = new TypeAndName(value);
		}
	}

	internal string Key => Name;

	public SchemaImporterExtensionElement()
	{
		properties.Add(name);
		properties.Add(type);
	}

	public SchemaImporterExtensionElement(string name, string type)
		: this()
	{
		Name = name;
		base[this.type] = new TypeAndName(type);
	}

	public SchemaImporterExtensionElement(string name, Type type)
		: this()
	{
		Name = name;
		Type = type;
	}
}

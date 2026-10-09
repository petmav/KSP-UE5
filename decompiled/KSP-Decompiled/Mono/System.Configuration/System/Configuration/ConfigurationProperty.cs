using System.ComponentModel;
using Unity;

namespace System.Configuration;

public sealed class ConfigurationProperty
{
	internal static readonly object NoDefaultValue = new object();

	private string name;

	private Type type;

	private object default_value;

	private TypeConverter converter;

	private ConfigurationValidatorBase validation;

	private ConfigurationPropertyOptions flags;

	private string description;

	private ConfigurationCollectionAttribute collectionAttribute;

	public TypeConverter Converter => converter;

	public object DefaultValue => default_value;

	public bool IsKey => (flags & ConfigurationPropertyOptions.IsKey) != 0;

	public bool IsRequired => (flags & ConfigurationPropertyOptions.IsRequired) != 0;

	public bool IsDefaultCollection => (flags & ConfigurationPropertyOptions.IsDefaultCollection) != 0;

	public string Name => name;

	public string Description => description;

	public Type Type => type;

	public ConfigurationValidatorBase Validator => validation;

	internal bool IsElement => typeof(ConfigurationElement).IsAssignableFrom(type);

	internal ConfigurationCollectionAttribute CollectionAttribute
	{
		get
		{
			return collectionAttribute;
		}
		set
		{
			collectionAttribute = value;
		}
	}

	public bool IsAssemblyStringTransformationRequired
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsTypeStringTransformationRequired
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsVersionCheckRequired
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public ConfigurationProperty(string name, Type type)
		: this(name, type, NoDefaultValue, TypeDescriptor.GetConverter(type), new DefaultValidator(), ConfigurationPropertyOptions.None, null)
	{
	}

	public ConfigurationProperty(string name, Type type, object defaultValue)
		: this(name, type, defaultValue, TypeDescriptor.GetConverter(type), new DefaultValidator(), ConfigurationPropertyOptions.None, null)
	{
	}

	public ConfigurationProperty(string name, Type type, object defaultValue, ConfigurationPropertyOptions options)
		: this(name, type, defaultValue, TypeDescriptor.GetConverter(type), new DefaultValidator(), options, null)
	{
	}

	public ConfigurationProperty(string name, Type type, object defaultValue, TypeConverter typeConverter, ConfigurationValidatorBase validator, ConfigurationPropertyOptions options)
		: this(name, type, defaultValue, typeConverter, validator, options, null)
	{
	}

	public ConfigurationProperty(string name, Type type, object defaultValue, TypeConverter typeConverter, ConfigurationValidatorBase validator, ConfigurationPropertyOptions options, string description)
	{
		this.name = name;
		converter = ((typeConverter != null) ? typeConverter : TypeDescriptor.GetConverter(type));
		if (defaultValue != null)
		{
			if (defaultValue == NoDefaultValue)
			{
				defaultValue = Type.GetTypeCode(type) switch
				{
					TypeCode.Object => null, 
					TypeCode.String => string.Empty, 
					_ => Activator.CreateInstance(type), 
				};
			}
			else if (!type.IsAssignableFrom(defaultValue.GetType()))
			{
				if (!converter.CanConvertFrom(defaultValue.GetType()))
				{
					throw new ConfigurationErrorsException($"The default value for property '{name}' has a different type than the one of the property itself: expected {type} but was {defaultValue.GetType()}");
				}
				defaultValue = converter.ConvertFrom(defaultValue);
			}
		}
		default_value = defaultValue;
		flags = options;
		this.type = type;
		validation = ((validator != null) ? validator : new DefaultValidator());
		this.description = description;
	}

	internal object ConvertFromString(string value)
	{
		if (converter != null)
		{
			return converter.ConvertFromInvariantString(value);
		}
		throw new NotImplementedException();
	}

	internal string ConvertToString(object value)
	{
		if (converter != null)
		{
			return converter.ConvertToInvariantString(value);
		}
		throw new NotImplementedException();
	}

	internal void Validate(object value)
	{
		if (validation != null)
		{
			validation.Validate(value);
		}
	}
}

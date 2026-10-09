namespace System.Configuration;

public class SettingsProperty
{
	private string name;

	private Type propertyType;

	private SettingsProvider provider;

	private bool isReadOnly;

	private object defaultValue;

	private SettingsSerializeAs serializeAs;

	private SettingsAttributeDictionary attributes;

	private bool throwOnErrorDeserializing;

	private bool throwOnErrorSerializing;

	public virtual SettingsAttributeDictionary Attributes => attributes;

	public virtual object DefaultValue
	{
		get
		{
			return defaultValue;
		}
		set
		{
			defaultValue = value;
		}
	}

	public virtual bool IsReadOnly
	{
		get
		{
			return isReadOnly;
		}
		set
		{
			isReadOnly = value;
		}
	}

	public virtual string Name
	{
		get
		{
			return name;
		}
		set
		{
			name = value;
		}
	}

	public virtual Type PropertyType
	{
		get
		{
			return propertyType;
		}
		set
		{
			propertyType = value;
		}
	}

	public virtual SettingsProvider Provider
	{
		get
		{
			return provider;
		}
		set
		{
			provider = value;
		}
	}

	public virtual SettingsSerializeAs SerializeAs
	{
		get
		{
			return serializeAs;
		}
		set
		{
			serializeAs = value;
		}
	}

	public bool ThrowOnErrorDeserializing
	{
		get
		{
			return throwOnErrorDeserializing;
		}
		set
		{
			throwOnErrorDeserializing = value;
		}
	}

	public bool ThrowOnErrorSerializing
	{
		get
		{
			return throwOnErrorSerializing;
		}
		set
		{
			throwOnErrorSerializing = value;
		}
	}

	public SettingsProperty(SettingsProperty propertyToCopy)
		: this(propertyToCopy.Name, propertyToCopy.PropertyType, propertyToCopy.Provider, propertyToCopy.IsReadOnly, propertyToCopy.DefaultValue, propertyToCopy.SerializeAs, new SettingsAttributeDictionary(propertyToCopy.Attributes), propertyToCopy.ThrowOnErrorDeserializing, propertyToCopy.ThrowOnErrorSerializing)
	{
	}

	public SettingsProperty(string name)
		: this(name, null, null, isReadOnly: false, null, SettingsSerializeAs.String, new SettingsAttributeDictionary(), throwOnErrorDeserializing: false, throwOnErrorSerializing: false)
	{
	}

	public SettingsProperty(string name, Type propertyType, SettingsProvider provider, bool isReadOnly, object defaultValue, SettingsSerializeAs serializeAs, SettingsAttributeDictionary attributes, bool throwOnErrorDeserializing, bool throwOnErrorSerializing)
	{
		this.name = name;
		this.propertyType = propertyType;
		this.provider = provider;
		this.isReadOnly = isReadOnly;
		this.defaultValue = defaultValue;
		this.serializeAs = serializeAs;
		this.attributes = attributes;
		this.throwOnErrorDeserializing = throwOnErrorDeserializing;
		this.throwOnErrorSerializing = throwOnErrorSerializing;
	}
}

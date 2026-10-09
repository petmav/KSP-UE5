using System.ComponentModel;
using Unity;

namespace System.Configuration;

public sealed class PropertyInformation
{
	private bool isLocked;

	private bool isModified;

	private int lineNumber;

	private string source;

	private object val;

	private PropertyValueOrigin origin;

	private readonly ConfigurationElement owner;

	private readonly ConfigurationProperty property;

	public TypeConverter Converter => property.Converter;

	public object DefaultValue => property.DefaultValue;

	public string Description => property.Description;

	public bool IsKey => property.IsKey;

	[System.MonoTODO]
	public bool IsLocked
	{
		get
		{
			return isLocked;
		}
		internal set
		{
			isLocked = value;
		}
	}

	public bool IsModified
	{
		get
		{
			return isModified;
		}
		internal set
		{
			isModified = value;
		}
	}

	public bool IsRequired => property.IsRequired;

	public int LineNumber
	{
		get
		{
			return lineNumber;
		}
		internal set
		{
			lineNumber = value;
		}
	}

	public string Name => property.Name;

	public string Source
	{
		get
		{
			return source;
		}
		internal set
		{
			source = value;
		}
	}

	public Type Type => property.Type;

	public ConfigurationValidatorBase Validator => property.Validator;

	public object Value
	{
		get
		{
			if (origin == PropertyValueOrigin.Default)
			{
				if (!property.IsElement)
				{
					return DefaultValue;
				}
				ConfigurationElement configurationElement = (ConfigurationElement)Activator.CreateInstance(Type, nonPublic: true);
				configurationElement.InitFromProperty(this);
				if (owner != null && owner.IsReadOnly())
				{
					configurationElement.SetReadOnly();
				}
				val = configurationElement;
				origin = PropertyValueOrigin.Inherited;
			}
			return val;
		}
		set
		{
			val = value;
			isModified = true;
			origin = PropertyValueOrigin.SetHere;
		}
	}

	internal bool IsElement => property.IsElement;

	public PropertyValueOrigin ValueOrigin => origin;

	internal ConfigurationProperty Property => property;

	internal PropertyInformation(ConfigurationElement owner, ConfigurationProperty property)
	{
		if (owner == null)
		{
			throw new ArgumentNullException("owner");
		}
		if (property == null)
		{
			throw new ArgumentNullException("property");
		}
		this.owner = owner;
		this.property = property;
	}

	internal void Reset(PropertyInformation parentProperty)
	{
		if (parentProperty != null)
		{
			if (property.IsElement)
			{
				((ConfigurationElement)Value).Reset((ConfigurationElement)parentProperty.Value);
				return;
			}
			val = parentProperty.Value;
			origin = PropertyValueOrigin.Inherited;
		}
		else
		{
			origin = PropertyValueOrigin.Default;
		}
	}

	internal string GetStringValue()
	{
		return property.ConvertToString(Value);
	}

	internal void SetStringValue(string value)
	{
		val = property.ConvertFromString(value);
		if (!object.Equals(val, DefaultValue))
		{
			origin = PropertyValueOrigin.SetHere;
		}
	}

	internal PropertyInformation()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

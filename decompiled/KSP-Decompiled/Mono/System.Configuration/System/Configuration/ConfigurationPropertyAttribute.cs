namespace System.Configuration;

[AttributeUsage(AttributeTargets.Property)]
public sealed class ConfigurationPropertyAttribute : Attribute
{
	private string name;

	private object default_value = ConfigurationProperty.NoDefaultValue;

	private ConfigurationPropertyOptions flags;

	public bool IsKey
	{
		get
		{
			return (flags & ConfigurationPropertyOptions.IsKey) != 0;
		}
		set
		{
			if (value)
			{
				flags |= ConfigurationPropertyOptions.IsKey;
			}
			else
			{
				flags &= ~ConfigurationPropertyOptions.IsKey;
			}
		}
	}

	public bool IsDefaultCollection
	{
		get
		{
			return (flags & ConfigurationPropertyOptions.IsDefaultCollection) != 0;
		}
		set
		{
			if (value)
			{
				flags |= ConfigurationPropertyOptions.IsDefaultCollection;
			}
			else
			{
				flags &= ~ConfigurationPropertyOptions.IsDefaultCollection;
			}
		}
	}

	public object DefaultValue
	{
		get
		{
			return default_value;
		}
		set
		{
			default_value = value;
		}
	}

	public ConfigurationPropertyOptions Options
	{
		get
		{
			return flags;
		}
		set
		{
			flags = value;
		}
	}

	public string Name => name;

	public bool IsRequired
	{
		get
		{
			return (flags & ConfigurationPropertyOptions.IsRequired) != 0;
		}
		set
		{
			if (value)
			{
				flags |= ConfigurationPropertyOptions.IsRequired;
			}
			else
			{
				flags &= ~ConfigurationPropertyOptions.IsRequired;
			}
		}
	}

	public ConfigurationPropertyAttribute(string name)
	{
		this.name = name;
	}
}

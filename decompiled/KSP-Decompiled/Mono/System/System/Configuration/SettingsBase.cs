using System.ComponentModel;

namespace System.Configuration;

public abstract class SettingsBase
{
	private bool sync;

	private SettingsContext context;

	private SettingsPropertyCollection properties;

	private SettingsProviderCollection providers;

	private SettingsPropertyValueCollection values = new SettingsPropertyValueCollection();

	public virtual SettingsContext Context => context;

	[Browsable(false)]
	public bool IsSynchronized => sync;

	public virtual object this[string propertyName]
	{
		get
		{
			if (sync)
			{
				lock (this)
				{
					return GetPropertyValue(propertyName);
				}
			}
			return GetPropertyValue(propertyName);
		}
		set
		{
			if (sync)
			{
				lock (this)
				{
					SetPropertyValue(propertyName, value);
					return;
				}
			}
			SetPropertyValue(propertyName, value);
		}
	}

	public virtual SettingsPropertyCollection Properties => properties;

	public virtual SettingsPropertyValueCollection PropertyValues
	{
		get
		{
			if (sync)
			{
				lock (this)
				{
					return values;
				}
			}
			return values;
		}
	}

	public virtual SettingsProviderCollection Providers => providers;

	public void Initialize(SettingsContext context, SettingsPropertyCollection properties, SettingsProviderCollection providers)
	{
		this.context = context;
		this.properties = properties;
		this.providers = providers;
	}

	public virtual void Save()
	{
		if (sync)
		{
			lock (this)
			{
				SaveCore();
				return;
			}
		}
		SaveCore();
	}

	private void SaveCore()
	{
		foreach (SettingsProvider provider in Providers)
		{
			SettingsPropertyValueCollection settingsPropertyValueCollection = new SettingsPropertyValueCollection();
			foreach (SettingsPropertyValue propertyValue in PropertyValues)
			{
				if (propertyValue.Property.Provider == provider)
				{
					settingsPropertyValueCollection.Add(propertyValue);
				}
			}
			if (settingsPropertyValueCollection.Count > 0)
			{
				provider.SetPropertyValues(Context, settingsPropertyValueCollection);
			}
		}
	}

	public static SettingsBase Synchronized(SettingsBase settingsBase)
	{
		settingsBase.sync = true;
		return settingsBase;
	}

	private object GetPropertyValue(string propertyName)
	{
		SettingsProperty settingsProperty = null;
		if (Properties == null || (settingsProperty = Properties[propertyName]) == null)
		{
			throw new SettingsPropertyNotFoundException($"The settings property '{propertyName}' was not found");
		}
		if (values[propertyName] == null)
		{
			foreach (SettingsPropertyValue propertyValue in settingsProperty.Provider.GetPropertyValues(Context, Properties))
			{
				values.Add(propertyValue);
			}
		}
		return PropertyValues[propertyName].PropertyValue;
	}

	private void SetPropertyValue(string propertyName, object value)
	{
		SettingsProperty settingsProperty = null;
		if (Properties == null || (settingsProperty = Properties[propertyName]) == null)
		{
			throw new SettingsPropertyNotFoundException($"The settings property '{propertyName}' was not found");
		}
		if (settingsProperty.IsReadOnly)
		{
			throw new SettingsPropertyIsReadOnlyException($"The settings property '{propertyName}' is read only");
		}
		if (settingsProperty.PropertyType != value.GetType())
		{
			throw new SettingsPropertyWrongTypeException($"The value supplied is of a type incompatible with the settings property '{propertyName}'");
		}
		PropertyValues[propertyName].PropertyValue = value;
	}
}

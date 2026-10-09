using System.ComponentModel;
using System.Reflection;
using System.Threading;

namespace System.Configuration;

public abstract class ApplicationSettingsBase : SettingsBase, INotifyPropertyChanged
{
	private string settingsKey;

	private SettingsContext context;

	private SettingsPropertyCollection properties;

	private ISettingsProviderService providerService;

	private SettingsPropertyValueCollection propertyValues;

	private SettingsProviderCollection providers;

	[Browsable(false)]
	public override SettingsContext Context
	{
		get
		{
			if (base.IsSynchronized)
			{
				Monitor.Enter(this);
			}
			try
			{
				if (context == null)
				{
					context = new SettingsContext();
					context["SettingsKey"] = "";
					Type type = GetType();
					context["GroupName"] = type.FullName;
					context["SettingsClassType"] = type;
				}
				return context;
			}
			finally
			{
				if (base.IsSynchronized)
				{
					Monitor.Exit(this);
				}
			}
		}
	}

	[System.MonoTODO]
	public override object this[string propertyName]
	{
		get
		{
			if (base.IsSynchronized)
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
			SettingsProperty settingsProperty = Properties[propertyName];
			if (settingsProperty == null)
			{
				throw new SettingsPropertyNotFoundException(propertyName);
			}
			if (settingsProperty.IsReadOnly)
			{
				throw new SettingsPropertyIsReadOnlyException(propertyName);
			}
			if (value != null && !settingsProperty.PropertyType.IsAssignableFrom(value.GetType()))
			{
				throw new SettingsPropertyWrongTypeException(propertyName);
			}
			if (PropertyValues[propertyName] == null)
			{
				CacheValuesByProvider(settingsProperty.Provider);
			}
			SettingChangingEventArgs settingChangingEventArgs = new SettingChangingEventArgs(propertyName, GetType().FullName, settingsKey, value, cancel: false);
			OnSettingChanging(this, settingChangingEventArgs);
			if (!settingChangingEventArgs.Cancel)
			{
				PropertyValues[propertyName].PropertyValue = value;
				OnPropertyChanged(this, new PropertyChangedEventArgs(propertyName));
			}
		}
	}

	[Browsable(false)]
	public override SettingsPropertyCollection Properties
	{
		get
		{
			if (base.IsSynchronized)
			{
				Monitor.Enter(this);
			}
			try
			{
				if (properties == null)
				{
					SettingsProvider local_provider = null;
					properties = new SettingsPropertyCollection();
					Type type = GetType();
					SettingsProviderAttribute[] array = (SettingsProviderAttribute[])type.GetCustomAttributes(typeof(SettingsProviderAttribute), inherit: false);
					if (array != null && array.Length != 0)
					{
						SettingsProvider settingsProvider = (SettingsProvider)Activator.CreateInstance(Type.GetType(array[0].ProviderTypeName));
						settingsProvider.Initialize(null, null);
						if (settingsProvider != null && Providers[settingsProvider.Name] == null)
						{
							Providers.Add(settingsProvider);
							local_provider = settingsProvider;
						}
					}
					PropertyInfo[] array2 = type.GetProperties();
					foreach (PropertyInfo propertyInfo in array2)
					{
						SettingAttribute[] array3 = (SettingAttribute[])propertyInfo.GetCustomAttributes(typeof(SettingAttribute), inherit: false);
						if (array3 != null && array3.Length != 0)
						{
							CreateSettingsProperty(propertyInfo, properties, ref local_provider);
						}
					}
				}
				return properties;
			}
			finally
			{
				if (base.IsSynchronized)
				{
					Monitor.Exit(this);
				}
			}
		}
	}

	[Browsable(false)]
	public override SettingsPropertyValueCollection PropertyValues
	{
		get
		{
			if (base.IsSynchronized)
			{
				Monitor.Enter(this);
			}
			try
			{
				if (propertyValues == null)
				{
					propertyValues = new SettingsPropertyValueCollection();
				}
				return propertyValues;
			}
			finally
			{
				if (base.IsSynchronized)
				{
					Monitor.Exit(this);
				}
			}
		}
	}

	[Browsable(false)]
	public override SettingsProviderCollection Providers
	{
		get
		{
			if (base.IsSynchronized)
			{
				Monitor.Enter(this);
			}
			try
			{
				if (providers == null)
				{
					providers = new SettingsProviderCollection();
				}
				return providers;
			}
			finally
			{
				if (base.IsSynchronized)
				{
					Monitor.Exit(this);
				}
			}
		}
	}

	[Browsable(false)]
	public string SettingsKey
	{
		get
		{
			return settingsKey;
		}
		set
		{
			settingsKey = value;
		}
	}

	public event PropertyChangedEventHandler PropertyChanged;

	public event SettingChangingEventHandler SettingChanging;

	public event SettingsLoadedEventHandler SettingsLoaded;

	public event SettingsSavingEventHandler SettingsSaving;

	protected ApplicationSettingsBase()
	{
		Initialize(Context, Properties, Providers);
	}

	protected ApplicationSettingsBase(IComponent owner)
		: this(owner, string.Empty)
	{
	}

	protected ApplicationSettingsBase(string settingsKey)
	{
		this.settingsKey = settingsKey;
		Initialize(Context, Properties, Providers);
	}

	protected ApplicationSettingsBase(IComponent owner, string settingsKey)
	{
		if (owner == null)
		{
			throw new ArgumentNullException();
		}
		providerService = (ISettingsProviderService)owner.Site.GetService(typeof(ISettingsProviderService));
		this.settingsKey = settingsKey;
		Initialize(Context, Properties, Providers);
	}

	public object GetPreviousVersion(string propertyName)
	{
		throw new NotImplementedException();
	}

	public void Reload()
	{
		foreach (SettingsProvider provider in Providers)
		{
			CacheValuesByProvider(provider);
		}
	}

	public void Reset()
	{
		Reload();
		foreach (SettingsPropertyValue propertyValue in PropertyValues)
		{
			propertyValue.PropertyValue = propertyValue.Reset();
		}
	}

	public override void Save()
	{
		Context.CurrentSettings = this;
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
		Context.CurrentSettings = null;
	}

	public virtual void Upgrade()
	{
	}

	protected virtual void OnPropertyChanged(object sender, PropertyChangedEventArgs e)
	{
		if (this.PropertyChanged != null)
		{
			this.PropertyChanged(sender, e);
		}
	}

	protected virtual void OnSettingChanging(object sender, SettingChangingEventArgs e)
	{
		if (this.SettingChanging != null)
		{
			this.SettingChanging(sender, e);
		}
	}

	protected virtual void OnSettingsLoaded(object sender, SettingsLoadedEventArgs e)
	{
		if (this.SettingsLoaded != null)
		{
			this.SettingsLoaded(sender, e);
		}
	}

	protected virtual void OnSettingsSaving(object sender, CancelEventArgs e)
	{
		if (this.SettingsSaving != null)
		{
			this.SettingsSaving(sender, e);
		}
	}

	private void CacheValuesByProvider(SettingsProvider provider)
	{
		SettingsPropertyCollection settingsPropertyCollection = new SettingsPropertyCollection();
		foreach (SettingsProperty property in Properties)
		{
			if (property.Provider == provider)
			{
				settingsPropertyCollection.Add(property);
			}
		}
		if (settingsPropertyCollection.Count > 0)
		{
			foreach (SettingsPropertyValue propertyValue in provider.GetPropertyValues(Context, settingsPropertyCollection))
			{
				if (PropertyValues[propertyValue.Name] != null)
				{
					PropertyValues[propertyValue.Name].PropertyValue = propertyValue.PropertyValue;
				}
				else
				{
					PropertyValues.Add(propertyValue);
				}
			}
		}
		OnSettingsLoaded(this, new SettingsLoadedEventArgs(provider));
	}

	private void InitializeSettings(SettingsPropertyCollection settings)
	{
	}

	private object GetPropertyValue(string propertyName)
	{
		SettingsProperty settingsProperty = Properties[propertyName];
		if (settingsProperty == null)
		{
			throw new SettingsPropertyNotFoundException(propertyName);
		}
		if (propertyValues == null)
		{
			InitializeSettings(Properties);
		}
		if (PropertyValues[propertyName] == null)
		{
			CacheValuesByProvider(settingsProperty.Provider);
		}
		return PropertyValues[propertyName].PropertyValue;
	}

	private void CreateSettingsProperty(PropertyInfo prop, SettingsPropertyCollection properties, ref SettingsProvider local_provider)
	{
		SettingsAttributeDictionary settingsAttributeDictionary = new SettingsAttributeDictionary();
		SettingsProvider settingsProvider = null;
		object defaultValue = null;
		SettingsSerializeAs serializeAs = SettingsSerializeAs.String;
		bool flag = false;
		object[] customAttributes = prop.GetCustomAttributes(inherit: false);
		for (int i = 0; i < customAttributes.Length; i++)
		{
			Attribute attribute = (Attribute)customAttributes[i];
			if (attribute is SettingsProviderAttribute)
			{
				settingsProvider = (SettingsProvider)Activator.CreateInstance(Type.GetType(((SettingsProviderAttribute)attribute).ProviderTypeName));
				settingsProvider.Initialize(null, null);
			}
			else if (attribute is DefaultSettingValueAttribute)
			{
				defaultValue = ((DefaultSettingValueAttribute)attribute).Value;
			}
			else if (attribute is SettingsSerializeAsAttribute)
			{
				serializeAs = ((SettingsSerializeAsAttribute)attribute).SerializeAs;
				flag = true;
			}
			else if (attribute is ApplicationScopedSettingAttribute || attribute is UserScopedSettingAttribute)
			{
				settingsAttributeDictionary.Add(attribute.GetType(), attribute);
			}
			else
			{
				settingsAttributeDictionary.Add(attribute.GetType(), attribute);
			}
		}
		if (!flag)
		{
			TypeConverter converter = TypeDescriptor.GetConverter(prop.PropertyType);
			if (converter != null && (!converter.CanConvertFrom(typeof(string)) || !converter.CanConvertTo(typeof(string))))
			{
				serializeAs = SettingsSerializeAs.Xml;
			}
		}
		SettingsProperty settingsProperty = new SettingsProperty(prop.Name, prop.PropertyType, settingsProvider, isReadOnly: false, defaultValue, serializeAs, settingsAttributeDictionary, throwOnErrorDeserializing: false, throwOnErrorSerializing: false);
		if (providerService != null)
		{
			settingsProperty.Provider = providerService.GetSettingsProvider(settingsProperty);
		}
		if (settingsProvider == null)
		{
			if (local_provider == null)
			{
				local_provider = new LocalFileSettingsProvider();
				local_provider.Initialize(null, null);
			}
			settingsProperty.Provider = local_provider;
			settingsProvider = local_provider;
		}
		if (settingsProvider != null)
		{
			SettingsProvider settingsProvider2 = Providers[settingsProvider.Name];
			if (settingsProvider2 != null)
			{
				settingsProperty.Provider = settingsProvider2;
			}
		}
		properties.Add(settingsProperty);
		if (settingsProperty.Provider != null && Providers[settingsProperty.Provider.Name] == null)
		{
			Providers.Add(settingsProperty.Provider);
		}
	}
}

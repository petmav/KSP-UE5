using Unity;

namespace System.Configuration;

public abstract class SettingsBase
{
	public virtual SettingsContext Context
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool IsSynchronized
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public virtual object this[string propertyName]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public virtual SettingsPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public virtual SettingsPropertyValueCollection PropertyValues
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public virtual SettingsProviderCollection Providers
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected SettingsBase()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Initialize(SettingsContext context, SettingsPropertyCollection properties, SettingsProviderCollection providers)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void Save()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static SettingsBase Synchronized(SettingsBase settingsBase)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

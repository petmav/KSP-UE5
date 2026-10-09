using System.ComponentModel;
using Unity;

namespace System.Configuration;

public abstract class ApplicationSettingsBase : SettingsBase, INotifyPropertyChanged
{
	public override SettingsContext Context
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override object this[string propertyName]
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

	public override SettingsPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override SettingsPropertyValueCollection PropertyValues
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override SettingsProviderCollection Providers
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string SettingsKey
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

	public event PropertyChangedEventHandler PropertyChanged
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event SettingChangingEventHandler SettingChanging
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event SettingsLoadedEventHandler SettingsLoaded
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event SettingsSavingEventHandler SettingsSaving
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected ApplicationSettingsBase()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected ApplicationSettingsBase(IComponent owner)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected ApplicationSettingsBase(IComponent owner, string settingsKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected ApplicationSettingsBase(string settingsKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object GetPreviousVersion(string propertyName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual void OnPropertyChanged(object sender, PropertyChangedEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnSettingChanging(object sender, SettingChangingEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnSettingsLoaded(object sender, SettingsLoadedEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnSettingsSaving(object sender, CancelEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Reload()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Reset()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Save()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void Upgrade()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

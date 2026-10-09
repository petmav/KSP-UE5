using System.ComponentModel;
using Unity;

namespace System.Configuration;

public class SettingChangingEventArgs : CancelEventArgs
{
	public object NewValue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string SettingClass
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string SettingKey
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string SettingName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SettingChangingEventArgs(string settingName, string settingClass, string settingKey, object newValue, bool cancel)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

using System.ComponentModel;

namespace System.Configuration;

public class SettingChangingEventArgs : CancelEventArgs
{
	private string settingName;

	private string settingClass;

	private string settingKey;

	private object newValue;

	public string SettingName => settingName;

	public string SettingClass => settingClass;

	public string SettingKey => settingKey;

	public object NewValue => newValue;

	public SettingChangingEventArgs(string settingName, string settingClass, string settingKey, object newValue, bool cancel)
		: base(cancel)
	{
		this.settingName = settingName;
		this.settingClass = settingClass;
		this.settingKey = settingKey;
		this.newValue = newValue;
	}
}

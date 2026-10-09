using System;

namespace KSP.UI.Screens.Settings
{
	[AttributeUsage(AttributeTargets.Field, AllowMultiple = false)]
	public class SettingsValue : Attribute
	{
		public object defaultValue;

		public SettingsValue(object defaultValue)
		{
			this.defaultValue = defaultValue;
		}
	}
}

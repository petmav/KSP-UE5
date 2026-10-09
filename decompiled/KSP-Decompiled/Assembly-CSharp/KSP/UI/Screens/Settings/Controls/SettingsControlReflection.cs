using System;
using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsControlReflection : SettingsControlBase
	{
		public string settingName;

		public TextMeshProUGUI valueText;

		protected AccessorBase accessor;

		private object value;

		public object Value
		{
			get
			{
				return value;
			}
			set
			{
				this.value = value;
				ValueUpdated();
				SettingsScreen.Instance.BroadcastMessage("SettingChanged", settingName + ";" + value.ToString(), SendMessageOptions.DontRequireReceiver);
			}
		}

		private void Start()
		{
			OnStart();
			accessor = GetAccessor(settingName);
			OnRevert();
		}

		protected virtual AccessorBase GetAccessor(string settingName)
		{
			try
			{
				AccessorBase accessorBase = AccessorBase.Create(typeof(GameSettings), null, settingName);
				if (accessorBase == null && !base.IgnoreEmptySetting)
				{
					Debug.LogError("SettingControl '" + titleText.text + "': Cannot find GameSetting field named '" + settingName + "'", base.gameObject);
					return null;
				}
				return accessorBase;
			}
			catch (Exception ex)
			{
				Debug.LogError(ex.Message);
			}
			return null;
		}

		protected virtual void OnStart()
		{
		}

		public override void OnApply()
		{
			PreApply();
			SetValue();
		}

		protected virtual void PreApply()
		{
		}

		public override void OnRevert()
		{
			if (accessor != null)
			{
				GetValue();
				ValueInitialized();
				ValueUpdated();
			}
		}

		protected virtual void ValueInitialized()
		{
		}

		protected virtual void ValueUpdated()
		{
		}

		protected virtual void GetValue()
		{
			object obj = accessor.Value;
			if (obj != null && obj is ICloneable)
			{
				value = (obj as ICloneable).Clone();
			}
			else
			{
				value = obj;
			}
		}

		protected virtual void SetValue()
		{
			if (accessor != null)
			{
				accessor.Value = value;
			}
		}
	}
}

using Highlighting;
using UnityEngine;

namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsBoolTogglePPFX : SettingsControlReflection
	{
		public UIButtonToggle toggle;

		[SettingsValue("True")]
		public string valueEnabled = "";

		[SettingsValue("False")]
		public string valueDisabled = "";

		protected bool supported = true;

		protected bool settingsSupported = true;

		protected bool interactable = true;

		protected SettingsIntArrayButtons aaField;

		protected override void OnStart()
		{
			supported = HighlightingSystem.CheckSupported();
			SetupAdditionalFields();
			Update();
			toggle.onToggle.AddListener(OnToggled);
		}

		private void SetupAdditionalFields()
		{
			SettingsIntArrayButtons[] array = Object.FindObjectsOfType<SettingsIntArrayButtons>();
			int num = 0;
			int num2 = array.Length;
			while (true)
			{
				if (num < num2)
				{
					if (array[num].settingName == "ANTI_ALIASING")
					{
						break;
					}
					num++;
					continue;
				}
				return;
			}
			aaField = array[num];
		}

		private void OnToggled()
		{
			if (interactable)
			{
				base.Value = toggle.state;
			}
		}

		protected override void ValueInitialized()
		{
			toggle.SetState((bool)base.Value);
		}

		protected override void ValueUpdated()
		{
			if (valueText == null)
			{
				return;
			}
			if ((bool)base.Value)
			{
				if (!string.IsNullOrEmpty(valueEnabled))
				{
					valueText.text = valueEnabled;
				}
			}
			else if (!string.IsNullOrEmpty(valueEnabled))
			{
				valueText.text = valueDisabled;
			}
		}

		private void Update()
		{
			if (aaField != null)
			{
				settingsSupported = (int)aaField.Value > 0;
			}
			else
			{
				settingsSupported = GameSettings.ANTI_ALIASING > 0;
			}
			bool flag = settingsSupported && supported;
			if (interactable != flag)
			{
				if (flag)
				{
					toggle.SetState((bool)base.Value);
					toggle.interactable = true;
				}
				else
				{
					toggle.SetState(on: false);
					toggle.interactable = false;
				}
				interactable = flag;
			}
		}
	}
}

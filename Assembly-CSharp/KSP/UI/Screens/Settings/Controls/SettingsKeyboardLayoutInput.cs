using System.Collections.Generic;
using KSP.Localization;
using TMPro;
using UnityEngine.UI;

namespace KSP.UI.Screens.Settings.Controls
{
	public class SettingsKeyboardLayoutInput : SettingsControlBase
	{
		public TextMeshProUGUI layoutName;

		public Button nextButton;

		public Button prevButton;

		private int currentLayout;

		private Dictionary<string, ConfigNode> layoutCollection;

		private List<string> layoutNames;

		private ConfigNode layoutConfig;

		private string selectedLayout = "";

		protected static bool layoutChangeWarning;

		protected SettingsLayoutConfig settingsLayoutConfig;

		public ConfigNode LayoutConfig => layoutConfig;

		public string SelectedLayout => selectedLayout;

		private void Start()
		{
			layoutCollection = GameSettings.KeyboardLayouts;
			nextButton.onClick.AddListener(OnNext);
			prevButton.onClick.AddListener(OnPrev);
			layoutNames = new List<string>();
			foreach (KeyValuePair<string, ConfigNode> item in layoutCollection)
			{
				layoutNames.Add(item.Key);
			}
			if (layoutCollection.Count > 0)
			{
				string layout = (string.IsNullOrEmpty(GameSettings.CURRENT_LAYOUT_SETTINGS) ? GameSettings.DetectKeyboardLayout() : GameSettings.CURRENT_LAYOUT_SETTINGS);
				int num = layoutNames.FindIndex((string t) => t.Equals(layout));
				if (num >= 0)
				{
					currentLayout = num;
				}
				ChangeLayout();
			}
			else
			{
				layoutName.text = Localizer.Format("#autoLOC_6001208");
			}
		}

		public void Init(SettingsLayoutConfig config)
		{
			settingsLayoutConfig = config;
		}

		public void LoadCurrentLayout()
		{
			GameSettings.LoadLayoutKeyBindings(layoutNames[currentLayout]);
		}

		public void SetKeysLayout()
		{
			layoutConfig = layoutCollection[layoutNames[currentLayout]];
		}

		public override void OnApply()
		{
			if (layoutCollection.Count != 0)
			{
				GameSettings.CURRENT_LAYOUT_SETTINGS = layoutNames[currentLayout];
			}
		}

		public override void OnRevert()
		{
			layoutConfig = null;
			if (layoutCollection.Count != 0)
			{
				string layout = (string.IsNullOrEmpty(GameSettings.CURRENT_LAYOUT_SETTINGS) ? GameSettings.DetectKeyboardLayout() : GameSettings.CURRENT_LAYOUT_SETTINGS);
				int num = layoutNames.FindIndex((string t) => t.Equals(layout));
				if (num >= 0)
				{
					currentLayout = num;
				}
				ChangeLayout();
			}
		}

		public void CleanLayout()
		{
			layoutConfig = null;
		}

		private void ChangeLayout()
		{
			ConfigNode configNode = layoutCollection[layoutNames[currentLayout]];
			SettingsScreen.Instance.KeysMapConfig = configNode.GetNode("KEY_MAP");
			string text = (layoutName.text = configNode.GetNode("KEYBOARD_LAYOUT").GetValue("name"));
			selectedLayout = text;
			settingsLayoutConfig.OnLayoutChange();
		}

		private void OnNext()
		{
			if (layoutCollection.Count == 0)
			{
				return;
			}
			if (layoutChangeWarning)
			{
				currentLayout = (currentLayout + 1) % layoutCollection.Count;
				ChangeLayout();
				return;
			}
			settingsLayoutConfig.ShowLayoutChangePopUp(delegate(bool result)
			{
				if (result)
				{
					currentLayout = ((currentLayout - 1 < 0) ? (layoutCollection.Count - 1) : (currentLayout - 1));
					ChangeLayout();
					layoutChangeWarning = true;
				}
			});
		}

		private void OnPrev()
		{
			if (layoutCollection.Count == 0)
			{
				return;
			}
			if (layoutChangeWarning)
			{
				currentLayout = ((currentLayout - 1 < 0) ? (layoutCollection.Count - 1) : (currentLayout - 1));
				ChangeLayout();
				return;
			}
			settingsLayoutConfig.ShowLayoutChangePopUp(delegate(bool result)
			{
				if (result)
				{
					currentLayout = ((currentLayout - 1 < 0) ? (layoutCollection.Count - 1) : (currentLayout - 1));
					ChangeLayout();
					layoutChangeWarning = true;
				}
			});
		}
	}
}

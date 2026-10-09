using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI.Screens.Settings.Controls
{
	[RequireComponent(typeof(UIRadioButton))]
	public class SettingsScreenTab : SettingsControlBase
	{
		private List<SettingsScreenTab> spawnedTabs = new List<SettingsScreenTab>();

		public Button.ButtonClickedEvent OnSelect = new Button.ButtonClickedEvent();

		public Button.ButtonClickedEvent OnDeselect = new Button.ButtonClickedEvent();

		public List<SettingsScreenTab> SpawnedTabs => spawnedTabs;

		public SettingsWindow SpawnedWindow { get; set; }

		public RectTransform WindowTransform { get; set; }

		public bool StartTrue { get; set; }

		public UIRadioButton btn { get; set; }

		private void Awake()
		{
			btn = GetComponent<UIRadioButton>();
		}

		private void Start()
		{
			if (!StartTrue)
			{
				btn.SetState(UIRadioButton.State.False, UIRadioButton.CallType.APPLICATIONSILENT, null, popButtonsInGroup: false);
				DeselectTab(null, UIRadioButton.CallType.APPLICATIONSILENT);
			}
			btn.onTrue.AddListener(SelectTab);
			btn.onFalse.AddListener(DeselectTab);
		}

		private void SelectTab(PointerEventData data, UIRadioButton.CallType callType)
		{
			Activate(activate: true);
			OnSelect.Invoke();
		}

		private void DeselectTab(PointerEventData data, UIRadioButton.CallType callType)
		{
			Activate(activate: false);
			OnDeselect.Invoke();
		}

		private void Activate(bool activate)
		{
			if (spawnedTabs.Count > 0)
			{
				int i = 0;
				for (int count = SpawnedTabs.Count; i < count; i++)
				{
					SettingsScreenTab settingsScreenTab = SpawnedTabs[i];
					settingsScreenTab.gameObject.SetActive(activate);
					if (settingsScreenTab.btn.CurrentState == UIRadioButton.State.True && settingsScreenTab.WindowTransform != null)
					{
						settingsScreenTab.WindowTransform.gameObject.SetActive(activate);
					}
				}
			}
			else if (WindowTransform != null)
			{
				WindowTransform.gameObject.SetActive(activate);
			}
			if (activate)
			{
				SettingsScreen.Instance.LoadSelectables();
			}
		}
	}
}

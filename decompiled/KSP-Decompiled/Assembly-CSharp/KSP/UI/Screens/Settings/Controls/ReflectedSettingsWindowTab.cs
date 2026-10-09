using System.Collections.Generic;
using UnityEngine;

namespace KSP.UI.Screens.Settings.Controls
{
	public class ReflectedSettingsWindowTab : SettingsControlBase
	{
		public UIStateButton tabStateButton;

		public RectTransform layoutGroup;

		private List<GameObject> spawnedObjects = new List<GameObject>();

		public List<GameObject> SpawnedObjects => spawnedObjects;

		public void Start()
		{
			if (tabStateButton != null)
			{
				tabStateButton.onValueChanged.AddListener(ShowTab);
			}
			OnStart();
		}

		protected virtual void OnStart()
		{
		}

		public void ShowTab(UIStateButton btn)
		{
			ShowTab(btn.currentState == "Active");
		}

		public void ShowTab(bool state)
		{
			int i = 0;
			for (int count = spawnedObjects.Count; i < count; i++)
			{
				spawnedObjects[i].gameObject.SetActive(state);
			}
		}
	}
}

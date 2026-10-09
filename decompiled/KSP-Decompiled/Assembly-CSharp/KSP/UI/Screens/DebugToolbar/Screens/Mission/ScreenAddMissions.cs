using System.Collections.Generic;
using Expansions.Missions;
using Expansions.Missions.Runtime;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Mission
{
	public class ScreenAddMissions : MonoBehaviour
	{
		public ScreenMissionExistingItem itemPrefab;

		public RectTransform listParent;

		protected List<ScreenMissionExistingItem> items = new List<ScreenMissionExistingItem>();

		private bool dirty;

		private void Start()
		{
			GameEvents.onGameStateCreated.Add(OnGameStateCreated);
			GameEvents.onGameStatePostLoad.Add(OnGameStateLoaded);
			RefreshItems();
		}

		private void OnDestroy()
		{
			GameEvents.onGameStateCreated.Remove(OnGameStateCreated);
			GameEvents.onGameStatePostLoad.Remove(OnGameStateLoaded);
		}

		private void Update()
		{
			if (dirty)
			{
				RefreshItems();
				dirty = false;
			}
		}

		private void RefreshItems()
		{
			ClearItems();
			if (MissionSystem.Instance == null)
			{
				CreateError("Mission system is not available at this time!");
				return;
			}
			int i = 0;
			for (int count = MissionSystem.missions.Count; i < count; i++)
			{
				CreateItem(MissionSystem.missions[i]);
			}
		}

		private void ClearItems()
		{
			int i = 0;
			for (int count = items.Count; i < count; i++)
			{
				Object.Destroy(items[i].gameObject);
			}
			items.Clear();
		}

		private void CreateItem(Expansions.Missions.Mission mission)
		{
			ScreenMissionExistingItem screenMissionExistingItem = Object.Instantiate(itemPrefab);
			screenMissionExistingItem.transform.SetParent(listParent, worldPositionStays: false);
			screenMissionExistingItem.Setup(mission);
			items.Add(screenMissionExistingItem);
		}

		private void CreateError(string errorText)
		{
			ScreenMissionExistingItem screenMissionExistingItem = Object.Instantiate(itemPrefab);
			screenMissionExistingItem.transform.SetParent(listParent, worldPositionStays: false);
			screenMissionExistingItem.SetupError(errorText);
			items.Add(screenMissionExistingItem);
		}

		private void OnGameStateCreated(Game game)
		{
			dirty = true;
		}

		private void OnGameStateLoaded(ConfigNode node)
		{
			dirty = true;
		}
	}
}

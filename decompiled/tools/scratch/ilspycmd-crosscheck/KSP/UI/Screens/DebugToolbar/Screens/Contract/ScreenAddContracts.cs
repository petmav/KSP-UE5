using System;
using System.Collections.Generic;
using Contracts;
using KSP.Localization;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Contract
{
	public class ScreenAddContracts : MonoBehaviour
	{
		public ScreenContractNewItem itemPrefab;

		public RectTransform listParent;

		protected List<ScreenContractNewItem> items = new List<ScreenContractNewItem>();

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
			if (ContractSystem.ContractTypes == null)
			{
				CreateError(Localizer.Format("#autoLOC_7003271"));
				return;
			}
			int i = 0;
			for (int count = ContractSystem.ContractTypes.Count; i < count; i++)
			{
				CreateItem(ContractSystem.ContractTypes[i]);
			}
		}

		private void ClearItems()
		{
			int i = 0;
			for (int count = items.Count; i < count; i++)
			{
				UnityEngine.Object.Destroy(items[i].gameObject);
			}
			items.Clear();
		}

		private void CreateItem(Type type)
		{
			ScreenContractNewItem screenContractNewItem = UnityEngine.Object.Instantiate(itemPrefab);
			screenContractNewItem.transform.SetParent(listParent, worldPositionStays: false);
			screenContractNewItem.Setup(type);
			items.Add(screenContractNewItem);
		}

		private void CreateError(string errorText)
		{
			ScreenContractNewItem screenContractNewItem = UnityEngine.Object.Instantiate(itemPrefab);
			screenContractNewItem.transform.SetParent(listParent, worldPositionStays: false);
			screenContractNewItem.SetupError(errorText);
			items.Add(screenContractNewItem);
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

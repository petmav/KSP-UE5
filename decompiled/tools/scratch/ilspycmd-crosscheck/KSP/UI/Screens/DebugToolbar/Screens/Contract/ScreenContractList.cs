using System.Collections.Generic;
using Contracts;
using KSP.Localization;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Contract
{
	public class ScreenContractList : MonoBehaviour
	{
		public enum ScreenContractListMode
		{
			Active,
			Offered,
			Archived
		}

		public ScreenContractExistingItem itemPrefab;

		public RectTransform listParent;

		protected List<ScreenContractExistingItem> items = new List<ScreenContractExistingItem>();

		public ScreenContractListMode mode;

		private bool dirty;

		private void Start()
		{
			GameEvents.onGameStateCreated.Add(OnGameStateCreated);
			GameEvents.onGameStatePostLoad.Add(OnGameStateLoaded);
			GameEvents.Contract.onOffered.Add(OnContractsModified);
			GameEvents.Contract.onAccepted.Add(OnContractsModified);
			GameEvents.Contract.onDeclined.Add(OnContractsModified);
			GameEvents.Contract.onCompleted.Add(OnContractsModified);
			GameEvents.Contract.onFailed.Add(OnContractsModified);
			GameEvents.Contract.onCancelled.Add(OnContractsModified);
			GameEvents.Contract.onFinished.Add(OnContractsModified);
			GameEvents.Contract.onContractsLoaded.Add(OnContractsModified);
			GameEvents.Contract.onContractsListChanged.Add(OnContractsModified);
			RefreshItems();
		}

		private void OnDestroy()
		{
			GameEvents.onGameStateCreated.Remove(OnGameStateCreated);
			GameEvents.onGameStatePostLoad.Remove(OnGameStateLoaded);
			GameEvents.Contract.onOffered.Remove(OnContractsModified);
			GameEvents.Contract.onAccepted.Remove(OnContractsModified);
			GameEvents.Contract.onDeclined.Remove(OnContractsModified);
			GameEvents.Contract.onCompleted.Remove(OnContractsModified);
			GameEvents.Contract.onFailed.Remove(OnContractsModified);
			GameEvents.Contract.onCancelled.Remove(OnContractsModified);
			GameEvents.Contract.onFinished.Remove(OnContractsModified);
			GameEvents.Contract.onContractsLoaded.Remove(OnContractsModified);
			GameEvents.Contract.onContractsListChanged.Remove(OnContractsModified);
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
			if (ContractSystem.Instance == null)
			{
				CreateError(Localizer.Format("#autoLOC_7003271"));
				return;
			}
			List<Contracts.Contract> list = ((mode == ScreenContractListMode.Archived) ? ContractSystem.Instance.ContractsFinished : ContractSystem.Instance.Contracts);
			int i = 0;
			for (int count = list.Count; i < count; i++)
			{
				Contracts.Contract contract = list[i];
				switch (mode)
				{
				case ScreenContractListMode.Active:
					if (contract.ContractState == Contracts.Contract.State.Active)
					{
						CreateItem(contract);
					}
					break;
				case ScreenContractListMode.Offered:
					if (contract.ContractState == Contracts.Contract.State.Offered)
					{
						CreateItem(contract);
					}
					break;
				case ScreenContractListMode.Archived:
					if (contract.IsFinished())
					{
						CreateItem(contract);
					}
					break;
				}
			}
			if (items.Count <= 0)
			{
				switch (mode)
				{
				default:
					CreateError(Localizer.Format("#autoLOC_7003282"));
					break;
				case ScreenContractListMode.Active:
					CreateError(Localizer.Format("#autoLOC_7003279"));
					break;
				case ScreenContractListMode.Offered:
					CreateError(Localizer.Format("#autoLOC_7003280"));
					break;
				case ScreenContractListMode.Archived:
					CreateError(Localizer.Format("#autoLOC_7003281"));
					break;
				}
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

		private void CreateItem(Contracts.Contract contract)
		{
			ScreenContractExistingItem screenContractExistingItem = Object.Instantiate(itemPrefab);
			screenContractExistingItem.transform.SetParent(listParent, worldPositionStays: false);
			screenContractExistingItem.Setup(contract, this);
			items.Add(screenContractExistingItem);
		}

		private void CreateError(string errorText)
		{
			ScreenContractExistingItem screenContractExistingItem = Object.Instantiate(itemPrefab);
			screenContractExistingItem.transform.SetParent(listParent, worldPositionStays: false);
			screenContractExistingItem.SetupError(errorText);
			items.Add(screenContractExistingItem);
		}

		private void OnGameStateCreated(Game game)
		{
			dirty = true;
		}

		private void OnGameStateLoaded(ConfigNode node)
		{
			dirty = true;
		}

		private void OnContractsModified()
		{
			dirty = true;
		}

		private void OnContractsModified(Contracts.Contract contract)
		{
			dirty = true;
		}

		public void SetDirty()
		{
			dirty = true;
		}
	}
}

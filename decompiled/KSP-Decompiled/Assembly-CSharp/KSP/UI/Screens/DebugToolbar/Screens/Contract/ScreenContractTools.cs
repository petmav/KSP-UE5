using Contracts;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Contract
{
	public class ScreenContractTools : MonoBehaviour
	{
		public Button resetWeights;

		public Button regenerateCurrent;

		public Button clearCurrent;

		public Button clearFinished;

		private bool interactable;

		private void Start()
		{
			resetWeights.onClick.AddListener(OnResetWeightsClicked);
			regenerateCurrent.onClick.AddListener(OnRegenerateCurrentClicked);
			clearCurrent.onClick.AddListener(OnClearCurrentClicked);
			clearFinished.onClick.AddListener(OnClearFinishedClicked);
			SetInteractable(ContractSystem.Instance != null);
		}

		private void Update()
		{
			if (interactable && ContractSystem.Instance == null)
			{
				SetInteractable(value: false);
			}
			else if (!interactable && ContractSystem.Instance != null)
			{
				SetInteractable(value: true);
			}
		}

		private void SetInteractable(bool value)
		{
			interactable = value;
			resetWeights.interactable = interactable;
			regenerateCurrent.interactable = interactable;
			clearCurrent.interactable = interactable;
			clearFinished.interactable = interactable;
		}

		private void OnResetWeightsClicked()
		{
			ContractSystem.ResetWeights();
		}

		private void OnRegenerateCurrentClicked()
		{
			ContractSystem.Instance.RebuildContracts();
		}

		private void OnClearCurrentClicked()
		{
			ContractSystem.Instance.ClearContractsCurrent();
		}

		private void OnClearFinishedClicked()
		{
			ContractSystem.Instance.ClearContractsFinished();
		}
	}
}

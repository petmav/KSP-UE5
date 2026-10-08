using System;
using Contracts;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.DebugToolbar.Screens.Contract
{
	public class ScreenContractNewItem : MonoBehaviour
	{
		public TextMeshProUGUI titleText;

		public TextMeshProUGUI assemblytext;

		public Button trivialButton;

		public Button significantButton;

		public Button exceptionalButton;

		public Type ContractType { get; private set; }

		private void Start()
		{
			trivialButton.onClick.AddListener(OnTrivialClicked);
			significantButton.onClick.AddListener(OnSignificantClicked);
			exceptionalButton.onClick.AddListener(OnExceptionalClicked);
		}

		public void Setup(Type contractType)
		{
			if (!(contractType == null))
			{
				ContractType = contractType;
				titleText.text = contractType.Name;
				assemblytext.text = contractType.Assembly.FullName;
			}
		}

		private static bool GenerateContract(Type type, Contracts.Contract.ContractPrestige prestige)
		{
			if (ContractSystem.Instance == null)
			{
				UnityEngine.Debug.LogError("Contract of type '" + type.Name + "' could not be generated.");
				return false;
			}
			Contracts.Contract contract = ContractSystem.Instance.GenerateContract(UnityEngine.Random.Range(int.MinValue, int.MaxValue), prestige, type);
			if (contract != null)
			{
				ContractSystem.Instance.Contracts.Add(contract);
				contract.Offer();
				return true;
			}
			UnityEngine.Debug.LogError("Contract of type '" + type.Name + "' could not be generated.");
			return false;
		}

		public void SetupError(string errorText)
		{
			ContractType = null;
			titleText.text = "<color=red>" + errorText + "</color>";
			titleText.enableWordWrapping = false;
			titleText.overflowMode = TextOverflowModes.Overflow;
			assemblytext.text = string.Empty;
			trivialButton.gameObject.SetActive(value: false);
			significantButton.gameObject.SetActive(value: false);
			exceptionalButton.gameObject.SetActive(value: false);
		}

		private void OnTrivialClicked()
		{
			if (!(ContractType == null))
			{
				GenerateContract(ContractType, Contracts.Contract.ContractPrestige.Trivial);
			}
		}

		private void OnSignificantClicked()
		{
			if (!(ContractType == null))
			{
				GenerateContract(ContractType, Contracts.Contract.ContractPrestige.Significant);
			}
		}

		private void OnExceptionalClicked()
		{
			if (!(ContractType == null))
			{
				GenerateContract(ContractType, Contracts.Contract.ContractPrestige.Exceptional);
			}
		}
	}
}

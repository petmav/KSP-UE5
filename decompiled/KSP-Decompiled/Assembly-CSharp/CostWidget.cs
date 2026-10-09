using KSP.UI;
using TMPro;
using UnityEngine;

public class CostWidget : CurrencyWidget
{
	public TextMeshProUGUI text;

	private Vector3 textSizeDefault;

	public Color affordableColor;

	public Color unaffordableColor;

	public Color irrelevantColor;

	private void Awake()
	{
		textSizeDefault = text.transform.localScale;
		GameEvents.onEditorShipModified.Add(onShipModified);
		GameEvents.onEditorShipCrewModified.Add(OnCrewModified);
		BaseCrewAssignmentDialog.onCrewDialogChange.Add(onCrewDialogChange);
		GameEvents.onEditorPodDeleted.Add(onShipReset);
		GameEvents.onEditorRestart.Add(onShipReset);
		GameEvents.onGUILaunchScreenVesselSelected.Add(onCraftFileSelected);
		text.color = irrelevantColor;
	}

	private void OnDestroy()
	{
		GameEvents.onEditorShipModified.Remove(onShipModified);
		GameEvents.onEditorShipCrewModified.Remove(OnCrewModified);
		BaseCrewAssignmentDialog.onCrewDialogChange.Remove(onCrewDialogChange);
		GameEvents.onEditorPodDeleted.Remove(onShipReset);
		GameEvents.onEditorRestart.Remove(onShipReset);
		GameEvents.onGUILaunchScreenVesselSelected.Remove(onCraftFileSelected);
	}

	private void OnCrewModified(VesselCrewManifest vcm)
	{
		if (HighLogic.LoadedSceneIsEditor && (bool)EditorLogic.fetch && EditorLogic.fetch.ship != null)
		{
			onCostChange(EditorLogic.fetch.ship.GetShipCosts(out var _, out var _, vcm));
		}
	}

	private void onShipReset()
	{
		float num = 0f;
		text.text = KSPUtil.LocalizeNumber(num, "N0");
		if (Funding.Instance != null)
		{
			if ((double)num > Funding.Instance.Funds)
			{
				text.color = unaffordableColor;
			}
			else
			{
				text.color = affordableColor;
			}
		}
	}

	private void onShipModified(ShipConstruct ship)
	{
		onCostChange(ship.GetShipCosts(out var _, out var _, ShipConstruction.ShipManifest));
	}

	private void onCrewDialogChange(VesselCrewManifest vcm)
	{
		if (HighLogic.LoadedSceneIsEditor && (bool)EditorLogic.fetch && EditorLogic.fetch.ship != null)
		{
			onCostChange(EditorLogic.fetch.ship.GetShipCosts(out var _, out var _, ShipConstruction.ShipManifest));
		}
	}

	private void onCraftFileSelected(ShipTemplate template)
	{
		onCostChange(template.totalCost);
	}

	private void onCostChange(float vCost)
	{
		CurrencyModifierQuery currencyModifierQuery = CurrencyModifierQuery.RunQuery(TransactionReasons.VesselRollout, vCost, 0f, 0f);
		if (currencyModifierQuery.GetEffectDelta(Currency.Funds) == 0f)
		{
			text.transform.localScale = textSizeDefault;
			text.text = KSPUtil.LocalizeNumber(vCost, "N0");
		}
		else
		{
			vCost += currencyModifierQuery.GetEffectDelta(Currency.Funds);
			text.transform.localScale = new Vector3(textSizeDefault.x * 0.8f, textSizeDefault.y, textSizeDefault.z);
			text.text = KSPUtil.LocalizeNumber(vCost, "N0") + " " + currencyModifierQuery.GetEffectPercentageText(Currency.Funds, "N1", CurrencyModifierQuery.TextStyling.OnGUI_LessIsGood);
		}
		if (Funding.Instance != null)
		{
			if ((double)vCost > Funding.Instance.Funds)
			{
				text.color = unaffordableColor;
			}
			else
			{
				text.color = affordableColor;
			}
		}
	}

	public override void DelayedStart()
	{
		if (HighLogic.LoadedSceneIsEditor)
		{
			onShipModified(EditorLogic.fetch.ship);
		}
	}

	public override bool OnAboutToStart()
	{
		return HighLogic.CurrentGame != null;
	}
}

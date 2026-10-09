using KSP.Localization;
using Strategies;
using UnityEngine;

public class AdministrationFacility : SpaceCenterBuilding
{
	private bool dialogIsUp;

	public override bool IsOpen()
	{
		return StrategySystem.Instance != null;
	}

	protected override void OnClicked()
	{
		InputLockManager.SetControlLock(ControlTypes.KSC_ALL, "administrationFacility");
		if (HighLogic.CurrentGame.Parameters.SpaceCenter.CanGoToAdmin)
		{
			if (StrategySystem.Instance != null)
			{
				GameEvents.onGUIAdministrationFacilitySpawn.Fire();
				GameEvents.onGUIAdministrationFacilityDespawn.Add(onDialogClose);
				dialogIsUp = true;
				HighLightBuilding(mouseOverIcon: false);
			}
			else
			{
				facilityClosedDialog = PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("FacilityClosed", Localizer.Format("#autoLOC_7003200"), Localizer.Format("#autoLOC_6001644"), HighLogic.UISkin, new DialogGUIButton(Localizer.Format("#autoLOC_253299"), OnPopupWarningDismiss)), persistAcrossScenes: false, HighLogic.UISkin);
				facilityClosedDialog.OnDismiss = OnPopupWarningDismiss;
			}
		}
		else
		{
			facilityLockedDialog = PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("FacilityLocked", Localizer.Format("#autoLOC_7003201"), Localizer.Format("#autoLOC_6001644"), HighLogic.UISkin, new DialogGUIButton(Localizer.Format("#autoLOC_253299"), OnPopupWarningDismiss)), persistAcrossScenes: false, HighLogic.UISkin);
			facilityLockedDialog.OnDismiss = OnPopupWarningDismiss;
		}
	}

	private void OnPopupWarningDismiss()
	{
		InputLockManager.RemoveControlLock("administrationFacility");
	}

	private void onDialogClose()
	{
		GameEvents.onGUIAdministrationFacilityDespawn.Remove(onDialogClose);
		dialogIsUp = false;
		InputLockManager.RemoveControlLock("administrationFacility");
	}

	protected override void OnOnDestroy()
	{
		if (dialogIsUp)
		{
			GameEvents.onGUIAdministrationFacilityDespawn.Remove(onDialogClose);
		}
	}
}

using KSP.Localization;
using UnityEngine;

public class AstronautComplexFacility : SpaceCenterBuilding
{
	private bool dialogIsUp;

	public override bool IsOpen()
	{
		return HighLogic.CurrentGame.Mode != Game.Modes.SCENARIO_NON_RESUMABLE;
	}

	protected override void OnClicked()
	{
		if (HighLogic.CurrentGame.Parameters.SpaceCenter.CanGoToAstronautC)
		{
			if (HighLogic.CurrentGame.Mode != Game.Modes.SCENARIO_NON_RESUMABLE)
			{
				GameEvents.onGUIAstronautComplexSpawn.Add(HighLogic.CurrentGame.CrewRoster.CheckRosterRespawn);
				GameEvents.onGUIAstronautComplexSpawn.Fire();
				GameEvents.onGUIAstronautComplexDespawn.Add(onAstronautComplexDialogClose);
				InputLockManager.SetControlLock(ControlTypes.KSC_ALL, "astronautComplexFacility");
				dialogIsUp = true;
				HighLightBuilding(mouseOverIcon: false);
			}
			else
			{
				InputLockManager.SetControlLock(ControlTypes.KSC_ALL, "ACFacility");
				facilityClosedDialog = PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("FacilityClosed", Localizer.Format("#autoLOC_7003200"), Localizer.Format("#autoLOC_6001643"), HighLogic.UISkin, new DialogGUIButton(Localizer.Format("#autoLOC_253299"), OnPopupWarningDismiss)), persistAcrossScenes: false, HighLogic.UISkin);
				facilityClosedDialog.OnDismiss = OnPopupWarningDismiss;
			}
		}
		else
		{
			InputLockManager.SetControlLock(ControlTypes.KSC_ALL, "ACFacility");
			facilityLockedDialog = PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("FacilityLocked", Localizer.Format("#autoLOC_7003201"), Localizer.Format("#autoLOC_6001643"), HighLogic.UISkin, new DialogGUIButton(Localizer.Format("#autoLOC_253299"), OnPopupWarningDismiss)), persistAcrossScenes: false, HighLogic.UISkin);
			facilityLockedDialog.OnDismiss = OnPopupWarningDismiss;
		}
	}

	private void OnPopupWarningDismiss()
	{
		InputLockManager.RemoveControlLock("ACFacility");
	}

	private void onAstronautComplexDialogClose()
	{
		GameEvents.onGUIAstronautComplexDespawn.Remove(onAstronautComplexDialogClose);
		GameEvents.onGUIAstronautComplexSpawn.Remove(HighLogic.CurrentGame.CrewRoster.CheckRosterRespawn);
		dialogIsUp = false;
		InputLockManager.RemoveControlLock("astronautComplexFacility");
	}

	protected override void OnOnDestroy()
	{
		if (dialogIsUp)
		{
			GameEvents.onGUIAstronautComplexDespawn.Remove(onAstronautComplexDialogClose);
		}
	}
}

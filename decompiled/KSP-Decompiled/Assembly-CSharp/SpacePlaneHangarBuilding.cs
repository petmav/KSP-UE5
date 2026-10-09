using Expansions.Missions.Runtime;
using KSP.Localization;
using UnityEngine;

public class SpacePlaneHangarBuilding : SpaceCenterBuilding
{
	public override bool IsOpen()
	{
		return HighLogic.CurrentGame.Mode != Game.Modes.SCENARIO_NON_RESUMABLE;
	}

	protected override void OnClicked()
	{
		if (HighLogic.CurrentGame.Mode == Game.Modes.SCENARIO_NON_RESUMABLE)
		{
			return;
		}
		bool flag = false;
		if (!HighLogic.CurrentGame.Parameters.SpaceCenter.CanGoInSPH && HighLogic.CurrentGame.Mode == Game.Modes.MISSION && MissionSystem.Instance != null)
		{
			for (int i = 0; i < MissionSystem.missions.Count; i++)
			{
				if (MissionSystem.missions[i].situation.VesselsArePending)
				{
					flag = true;
				}
			}
		}
		if (HighLogic.CurrentGame.Parameters.SpaceCenter.CanGoInSPH || flag)
		{
			GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE);
			EditorFacility editorFacility = EditorFacility.None;
			if (ShipConstruction.ShipConfig != null)
			{
				editorFacility = ShipConstruction.ShipType;
			}
			MonoBehaviour.print("Going to Hangar! CachedShipType: " + editorFacility);
			EditorDriver.StartupBehaviour = ((editorFacility == EditorFacility.SPH) ? EditorDriver.StartupBehaviours.LOAD_FROM_CACHE : EditorDriver.StartupBehaviours.START_CLEAN);
			EditorDriver.StartEditor(EditorFacility.SPH);
		}
		else
		{
			InputLockManager.SetControlLock(ControlTypes.KSC_ALL, "SPHFacility");
			PopupDialog.SpawnPopupDialog(new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new MultiOptionDialog("FacilityLocked", Localizer.Format("#autoLOC_7003201"), Localizer.Format("#autoLOC_6001661"), HighLogic.UISkin, new DialogGUIButton(Localizer.Format("#autoLOC_253299"), OnPopupWarningDismiss)), persistAcrossScenes: false, HighLogic.UISkin).OnDismiss = OnPopupWarningDismiss;
		}
	}

	private void OnPopupWarningDismiss()
	{
		InputLockManager.RemoveControlLock("SPHFacility");
	}
}

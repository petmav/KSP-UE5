using System.Collections;
using PreFlightTests;
using UnityEngine;

namespace KSP.UI
{
	public class CrewAssignmentDialog : BaseCrewAssignmentDialog
	{
		public static CrewAssignmentDialog Instance;

		protected override void Awake()
		{
			if (Instance != null)
			{
				Debug.LogError("CrewAssignmentDialog: Instance already exists.");
				base.gameObject.DestroyGameObject();
				return;
			}
			Instance = this;
			if (!HighLogic.LoadedSceneIsEditor || (HighLogic.LoadedSceneIsEditor && HighLogic.CurrentGame.Mode == Game.Modes.MISSION_BUILDER))
			{
				base.CurrentCrewRoster = HighLogic.CurrentGame.CrewRoster;
			}
			base.Awake();
			GameEvents.onGUIAstronautComplexDespawn.Add(Refresh);
		}

		protected override void OnDestroy()
		{
			base.OnDestroy();
			GameEvents.onGUIAstronautComplexDespawn.Remove(Refresh);
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}

		protected override void SetCurrentCrewRoster(KerbalRoster newRoster)
		{
			HighLogic.CurrentGame.CrewRoster = newRoster;
		}

		protected override KerbalRoster GetCurrentCrewRoster()
		{
			return HighLogic.CurrentGame.CrewRoster;
		}

		public void ButtonAstronautComplex()
		{
			onOpenAstronautComplex();
		}

		private void onOpenAstronautComplex()
		{
			InputLockManager.SetControlLock("ACoperationalCheck");
			PreFlightCheck preFlightCheck = new PreFlightCheck(onOpenACProceed, onOpenACDismiss);
			preFlightCheck.AddTest(new FacilityOperational("AstronautComplex", "Astronaut Complex"));
			preFlightCheck.RunTests();
		}

		private void onOpenACProceed()
		{
			onOpenACDismiss();
			GameEvents.onGUIAstronautComplexSpawn.Fire();
		}

		private void onOpenACDismiss()
		{
			InputLockManager.RemoveControlLock("ACoperationalCheck");
		}

		protected override void MoveCrewToEmptySeat(UIList fromlist, UIList tolist, UIListItem itemToMove, int index)
		{
			base.MoveCrewToEmptySeat(fromlist, tolist, itemToMove, index);
			EditorLogicUpdateCrew();
		}

		protected override void MoveCrewToAvail(UIList fromlist, UIList tolist, UIListItem itemToMove)
		{
			base.MoveCrewToAvail(fromlist, tolist, itemToMove);
			EditorLogicUpdateCrew();
		}

		protected override void DropOnCrewList(UIList fromList, UIListItem insertItem, int insertIndex)
		{
			base.DropOnCrewList(fromList, insertItem, insertIndex);
			StartCoroutine(RefreshCrewOnDrop());
		}

		protected override void DropOnAvailList(UIList fromList, UIListItem insertItem, int insertIndex)
		{
			base.DropOnAvailList(fromList, insertItem, insertIndex);
			StartCoroutine(RefreshCrewOnDrop());
		}

		private IEnumerator RefreshCrewOnDrop()
		{
			yield return null;
			EditorLogicUpdateCrew();
		}

		private void EditorLogicUpdateCrew()
		{
			if (EditorLogic.fetch != null)
			{
				EditorLogic.fetch.UpdateCrewManifest();
			}
			Refresh();
		}
	}
}

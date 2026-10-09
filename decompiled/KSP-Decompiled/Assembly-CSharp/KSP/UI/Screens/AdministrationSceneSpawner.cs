using UnityEngine;

namespace KSP.UI.Screens
{
	public class AdministrationSceneSpawner : MonoBehaviour
	{
		public UICanvasPrefab AdministrationScreenPrefab;

		private void Awake()
		{
			GameEvents.onGUIAdministrationFacilitySpawn.Add(onAdminSpawn);
			GameEvents.onGUIAdministrationFacilityDespawn.Add(onAdminDespawn);
		}

		private void OnDestroy()
		{
			GameEvents.onGUIAdministrationFacilitySpawn.Remove(onAdminSpawn);
			GameEvents.onGUIAdministrationFacilityDespawn.Remove(onAdminDespawn);
		}

		private void onAdminSpawn()
		{
			UIMasterController.Instance.AddCanvas(AdministrationScreenPrefab);
			MusicLogic.fetch.PauseWithCrossfade(MusicLogic.AdditionalThemes.Administration);
		}

		private void onAdminDespawn()
		{
			GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE);
			Debug.Log(AdministrationScreenPrefab);
			UIMasterController.Instance.RemoveCanvas(AdministrationScreenPrefab);
			MusicLogic.fetch.UnpauseWithCrossfade();
		}
	}
}

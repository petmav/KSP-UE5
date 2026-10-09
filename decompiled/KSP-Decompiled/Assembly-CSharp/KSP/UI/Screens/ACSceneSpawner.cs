using UnityEngine;

namespace KSP.UI.Screens
{
	public class ACSceneSpawner : MonoBehaviour
	{
		public UICanvasPrefab ACScreenPrefab;

		private void Awake()
		{
			GameEvents.onGUIAstronautComplexSpawn.Add(onACSpawn);
			GameEvents.onGUIAstronautComplexDespawn.Add(onACDespawn);
		}

		private void OnDestroy()
		{
			GameEvents.onGUIAstronautComplexSpawn.Remove(onACSpawn);
			GameEvents.onGUIAstronautComplexDespawn.Remove(onACDespawn);
		}

		private void onACSpawn()
		{
			UIMasterController.Instance.AddCanvas(ACScreenPrefab);
			MusicLogic.fetch.PauseWithCrossfade(MusicLogic.AdditionalThemes.AstronautComplex);
		}

		private void onACDespawn()
		{
			GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE);
			UIMasterController.Instance.RemoveCanvas(ACScreenPrefab);
			MusicLogic.fetch.UnpauseWithCrossfade();
		}
	}
}

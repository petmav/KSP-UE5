using UnityEngine;
using UnityEngine.Rendering;

namespace KSP.UI.Screens
{
	public class RDSceneSpawner : MonoBehaviour
	{
		public UICanvasPrefab RDScreenPrefab;

		public Cubemap RDSceneReflection;

		private DefaultReflectionMode oldReflectionMode;

		private Cubemap oldReflection;

		private void Awake()
		{
			GameEvents.onGUIRnDComplexSpawn.Add(onRDSpawn);
			GameEvents.onGUIRnDComplexDespawn.Add(onRDDespawn);
		}

		private void OnDestroy()
		{
			GameEvents.onGUIRnDComplexSpawn.Remove(onRDSpawn);
			GameEvents.onGUIRnDComplexDespawn.Remove(onRDDespawn);
		}

		private void onRDSpawn()
		{
			UIMasterController.Instance.AddCanvas(RDScreenPrefab);
			oldReflectionMode = RenderSettings.defaultReflectionMode;
			RenderSettings.defaultReflectionMode = DefaultReflectionMode.Custom;
			oldReflection = RenderSettings.customReflection;
			RenderSettings.customReflection = RDSceneReflection;
			MusicLogic.fetch.PauseWithCrossfade(MusicLogic.AdditionalThemes.ResearchAndDevelopment);
		}

		private void onRDDespawn()
		{
			GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE);
			UIMasterController.Instance.RemoveCanvas(RDScreenPrefab);
			RenderSettings.defaultReflectionMode = oldReflectionMode;
			RenderSettings.customReflection = oldReflection;
			MusicLogic.fetch.UnpauseWithCrossfade();
		}
	}
}

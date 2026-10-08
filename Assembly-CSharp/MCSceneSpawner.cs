using KSP.UI;
using UnityEngine;

public class MCSceneSpawner : MonoBehaviour
{
	public UICanvasPrefab missionControlPrefab;

	private void Awake()
	{
		GameEvents.onGUIMissionControlSpawn.Add(OnMCSpawn);
		GameEvents.onGUIMissionControlDespawn.Add(OnMCDespawn);
	}

	private void OnDestroy()
	{
		GameEvents.onGUIMissionControlSpawn.Remove(OnMCSpawn);
		GameEvents.onGUIMissionControlDespawn.Remove(OnMCDespawn);
	}

	private void OnMCSpawn()
	{
		UIMasterController.Instance.AddCanvas(missionControlPrefab);
		MusicLogic.fetch.PauseWithCrossfade(MusicLogic.AdditionalThemes.MissionControl);
	}

	private void OnMCDespawn()
	{
		GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE);
		MusicLogic.fetch.UnpauseWithCrossfade();
		UIMasterController.Instance.RemoveCanvas(missionControlPrefab);
	}
}

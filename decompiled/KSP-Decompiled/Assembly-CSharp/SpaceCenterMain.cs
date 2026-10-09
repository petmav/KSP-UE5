using System.Collections;
using System.Collections.Generic;
using KSP.UI;
using UnityEngine;

public class SpaceCenterMain : MonoBehaviour
{
	public UICanvasPrefab spaceCenterCanvas;

	public bool bypassLoadingEnforce;

	public List<string> gameObjectsToDisable;

	private List<GameObject> gameObjectsFound;

	private IEnumerator Start()
	{
		UIMasterController.Instance.AddCanvas(spaceCenterCanvas);
		Game game = GamePersistence.LoadGame("persistent", HighLogic.SaveFolder, nullIfIncompatible: true, suppressIncompatibleMessage: false);
		if (game != null)
		{
			game.Load();
			if (game.flightState == null)
			{
				for (int i = 0; i < 10; i++)
				{
					yield return null;
				}
				GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE);
			}
		}
		else
		{
			GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE);
		}
		GameEvents.onGameSceneLoadRequested.Add(OnGameSceneChange);
		gameObjectsFound = new List<GameObject>();
		foreach (string item in gameObjectsToDisable)
		{
			GameObject gameObject = GameObject.Find(item);
			if (gameObject == null)
			{
				Debug.Log("SpaceCenterMain: Cannot find gameObject of url " + item);
				continue;
			}
			gameObjectsFound.Add(gameObject);
			gameObject.SetActive(value: false);
		}
	}

	private void OnDestroy()
	{
		GameEvents.onGameSceneLoadRequested.Remove(OnGameSceneChange);
		InputLockManager.RemoveControlLock("ksc_ApplicationFocus");
	}

	private void OnApplicationFocus(bool focus)
	{
		if (!focus)
		{
			InputLockManager.SetControlLock("ksc_ApplicationFocus");
		}
		else
		{
			InputLockManager.RemoveControlLock("ksc_ApplicationFocus");
		}
	}

	private void onInputLockModified(GameEvents.FromToAction<ControlTypes, ControlTypes> ctrls)
	{
		Planetarium.fetch.pause = InputLockManager.IsLocked(ControlTypes.KSC_UI) && InputLockManager.IsLocked(ControlTypes.KSC_FACILITIES) && InputLockManager.IsLocked(ControlTypes.TIMEWARP);
	}

	private void OnGameSceneChange(GameScenes scene)
	{
		GameEvents.onInputLocksModified.Remove(onInputLockModified);
		foreach (GameObject item in gameObjectsFound)
		{
			if (item == null)
			{
				Debug.Log("SpaceCenterMain: Game Object is now null! ");
			}
			else
			{
				item.SetActive(value: true);
			}
		}
	}
}

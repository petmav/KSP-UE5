using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class VesselRetrieval : MonoBehaviour
{
	private int KSCFrameDelay = 8;

	private HashSet<Guid> IDsOfVesselsToRecover;

	private void Awake()
	{
		IDsOfVesselsToRecover = new HashSet<Guid>();
		GameEvents.OnVesselRecoveryRequested.Add(onVesselRecoveryRequested);
		SceneManager.sceneLoaded += OnSceneLoaded;
	}

	private void OnDestroy()
	{
		GameEvents.OnVesselRecoveryRequested.Remove(onVesselRecoveryRequested);
		SceneManager.sceneLoaded -= OnSceneLoaded;
	}

	private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
	{
		StartCoroutine(OnLevelLoaded(HighLogic.GetLoadedGameSceneFromBuildIndex(scene.buildIndex)));
	}

	private IEnumerator OnLevelLoaded(GameScenes scene)
	{
		switch (scene)
		{
		case GameScenes.SPACECENTER:
			if (IDsOfVesselsToRecover.Count > 0)
			{
				int i = 0;
				while (i < KSCFrameDelay)
				{
					yield return null;
					int num = i + 1;
					i = num;
				}
				recoverVessels();
			}
			break;
		}
	}

	private void onVesselRecoveryRequested(Vessel v)
	{
		IDsOfVesselsToRecover.Add(v.id);
		GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE);
		HighLogic.LoadScene(GameScenes.SPACECENTER);
	}

	private void recoverVessels()
	{
		if (IDsOfVesselsToRecover.Count > 0)
		{
			List<Vessel> vesselsToRecover = getVesselsToRecover(IDsOfVesselsToRecover);
			int count = vesselsToRecover.Count;
			for (int i = 0; i < count; i++)
			{
				recoverVessel(vesselsToRecover[i]);
			}
			GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE);
			IDsOfVesselsToRecover.Clear();
		}
	}

	private List<Vessel> getVesselsToRecover(HashSet<Guid> vesselIDs)
	{
		List<Vessel> list = new List<Vessel>();
		int count = FlightGlobals.Vessels.Count;
		for (int i = 0; i < count; i++)
		{
			Vessel vessel = FlightGlobals.Vessels[i];
			if (vesselIDs.Contains(vessel.id))
			{
				list.Add(vessel);
			}
		}
		return list;
	}

	private void recoverVessel(Vessel v)
	{
		GameEvents.onVesselRecovered.Fire(v.protoVessel, data1: false);
		UnityEngine.Object.DestroyImmediate(v.gameObject);
	}
}

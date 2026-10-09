using System.Collections.Generic;
using UnityEngine;

namespace KSP.UI.Screens
{
	public class KSCVesselMarkers : MonoBehaviour
	{
		private List<KSCVesselMarker> markers;

		public static KSCVesselMarkers fetch;

		private void Awake()
		{
			fetch = null;
			GameEvents.onGUILaunchScreenSpawn.Add(onGUIVesselScreenSpawn);
			GameEvents.onGUIRnDComplexSpawn.Add(onGUIScreenSpawn);
			GameEvents.onGUIAstronautComplexSpawn.Add(onGUIScreenSpawn);
			GameEvents.onGUIMissionControlSpawn.Add(onGUIScreenSpawn);
			GameEvents.onGUILaunchScreenDespawn.Add(onGUIScreenDespawn);
			GameEvents.onGUIRnDComplexDespawn.Add(onGUIScreenDespawn);
			GameEvents.onGUIAstronautComplexDespawn.Add(onGUIACScreenDespawn);
			GameEvents.onGUIMissionControlDespawn.Add(onGUIScreenDespawn);
			GameEvents.onGameSceneLoadRequested.Add(onSceneLoadRequested);
			GameEvents.onVesselWillDestroy.Add(onVesselDestroy);
			markers = new List<KSCVesselMarker>();
			StartCoroutine(CallbackUtil.DelayedCallback(15, SpawnVesselMarkers));
			fetch = this;
		}

		private void SpawnVesselMarkers()
		{
			int count = FlightGlobals.Vessels.Count;
			for (int i = 0; i < count; i++)
			{
				Vessel vessel = FlightGlobals.Vessels[i];
				if (vessel != null && vessel.LandedOrSplashed && vessel.mainBody == Planetarium.fetch.Home && vessel.vesselType != VesselType.DeployedSciencePart && vessel.vesselType != VesselType.DroppedPart)
				{
					markers.Add(KSCVesselMarker.Create(vessel, OnMarkerDismiss));
				}
			}
		}

		private void ClearVesselMarkers()
		{
			int count = markers.Count;
			for (int i = 0; i < count; i++)
			{
				markers[i].Terminate();
			}
			markers.Clear();
		}

		private void OnMarkerDismiss(Vessel v, KSCVesselMarker.DismissAction dma)
		{
			switch (dma)
			{
			case KSCVesselMarker.DismissAction.Fly:
				StartCoroutine(CallbackUtil.DelayedCallback(1, FlyVessel, v));
				break;
			case KSCVesselMarker.DismissAction.Recover:
				StartCoroutine(CallbackUtil.DelayedCallback(1, RecoverVessel, v));
				break;
			case KSCVesselMarker.DismissAction.None:
				break;
			}
		}

		private void FlyVessel(Vessel v)
		{
			GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE, GameScenes.FLIGHT);
			FlightDriver.StartAndFocusVessel("persistent", FlightGlobals.Vessels.IndexOf(v));
			ClearVesselMarkers();
		}

		private void RecoverVessel(Vessel v)
		{
			ShipConstruction.RecoverVesselFromFlight(v.protoVessel, HighLogic.CurrentGame.flightState);
			GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE, GameScenes.SPACECENTER);
			StartCoroutine(CallbackUtil.DelayedCallback(1, RefreshMarkers));
		}

		private void onVesselDestroy(Vessel v)
		{
			RefreshMarkers();
		}

		public void RefreshMarkers()
		{
			ClearVesselMarkers();
			SpawnVesselMarkers();
		}

		private void OnDestroy()
		{
			GameEvents.onGUILaunchScreenSpawn.Remove(onGUIVesselScreenSpawn);
			GameEvents.onGUIRnDComplexSpawn.Remove(onGUIScreenSpawn);
			GameEvents.onGUIAstronautComplexSpawn.Remove(onGUIScreenSpawn);
			GameEvents.onGUIMissionControlSpawn.Remove(onGUIScreenSpawn);
			GameEvents.onGUILaunchScreenDespawn.Remove(onGUIScreenDespawn);
			GameEvents.onGUIRnDComplexDespawn.Remove(onGUIScreenDespawn);
			GameEvents.onGUIAstronautComplexDespawn.Remove(onGUIACScreenDespawn);
			GameEvents.onGUIMissionControlDespawn.Remove(onGUIScreenDespawn);
			GameEvents.onGameSceneLoadRequested.Remove(onSceneLoadRequested);
			GameEvents.onVesselWillDestroy.Remove(onVesselDestroy);
			if (fetch != null && fetch == this)
			{
				fetch = null;
			}
		}

		private void onGUIVesselScreenSpawn(GameEvents.VesselSpawnInfo info)
		{
			onGUIScreenSpawn();
		}

		private void onGUIScreenSpawn()
		{
			ClearVesselMarkers();
		}

		private void onGUIACScreenDespawn()
		{
			if (HighLogic.LoadedScene == GameScenes.SPACECENTER && (!(VesselSpawnDialog.Instance != null) || !VesselSpawnDialog.Instance.Visible))
			{
				SpawnVesselMarkers();
			}
		}

		private void onGUIScreenDespawn()
		{
			if (HighLogic.LoadedScene == GameScenes.SPACECENTER && markers.Count == 0)
			{
				SpawnVesselMarkers();
			}
		}

		private void onSceneLoadRequested(GameScenes scn)
		{
			ClearVesselMarkers();
			Object.Destroy(this);
		}
	}
}

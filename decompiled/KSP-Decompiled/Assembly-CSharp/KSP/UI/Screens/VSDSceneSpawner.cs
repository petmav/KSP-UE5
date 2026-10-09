using UnityEngine;

namespace KSP.UI.Screens
{
	public class VSDSceneSpawner : MonoBehaviour
	{
		public UICanvasPrefab VSDScreenPrefab;

		private void Awake()
		{
			GameEvents.onGUILaunchScreenSpawn.Add(onVSDSpawn);
			GameEvents.onGUILaunchScreenDespawn.Add(onVSDDespawn);
		}

		private void OnDestroy()
		{
			GameEvents.onGUILaunchScreenSpawn.Remove(onVSDSpawn);
			GameEvents.onGUILaunchScreenDespawn.Remove(onVSDDespawn);
		}

		private void onVSDSpawn(GameEvents.VesselSpawnInfo info)
		{
			EditorDriver.editorFacility = info.callingFacility.facilityType;
			EditorDriver.setupValidLaunchSites();
			UIMasterController.Instance.AddCanvas(VSDScreenPrefab);
			StartCoroutine(CallbackUtil.DelayedCallback(1, VesselSpawnDialog.Instance.InitiateGUI, info));
		}

		private void onVSDDespawn()
		{
			UIMasterController.Instance.RemoveCanvas(VSDScreenPrefab);
		}
	}
}

using System;
using System.Collections.Generic;
using UnityEngine;

namespace KSP.UI.Screens
{
	public class UIAppSpawner : MonoBehaviour
	{
		[Serializable]
		public class AppWrapper
		{
			public GameObject prefab;

			public List<GameScenes> scenes;

			[NonSerialized]
			public GameObject instantiatedApp;
		}

		public UICanvasPrefab ApplauncherScreenPrefab;

		public AppWrapper[] apps;

		private void Awake()
		{
			GameEvents.onLevelWasLoaded.Add(OnLevelLoaded);
		}

		private void OnDestroy()
		{
			GameEvents.onLevelWasLoaded.Remove(OnLevelLoaded);
		}

		private void OnLevelLoaded(GameScenes scene)
		{
			if ((scene == GameScenes.EDITOR || scene == GameScenes.FLIGHT || scene == GameScenes.MAINMENU || scene == GameScenes.SPACECENTER || scene == GameScenes.TRACKSTATION) && ApplicationLauncher.Instance == null)
			{
				UIMasterController.Instance.AddCanvas(UIMasterController.Instance.appCanvas, ApplauncherScreenPrefab, removeOnSceneSwitch: false);
			}
			int i = 0;
			for (int num = apps.Length; i < num; i++)
			{
				AppWrapper appWrapper = apps[i];
				if (appWrapper.scenes.Contains(HighLogic.LoadedScene) && appWrapper.instantiatedApp == null)
				{
					appWrapper.instantiatedApp = UnityEngine.Object.Instantiate(appWrapper.prefab);
				}
			}
		}
	}
}

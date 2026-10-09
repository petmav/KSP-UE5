using UnityEngine;

public class SpaceCenterSurfaceObject : MonoBehaviour
{
	private SurfaceObject srfObj;

	private CelestialBody cb;

	private bool setup;

	private void Awake()
	{
		GameEvents.onLevelWasLoaded.Add(OnGameSceneLoaded);
		GameEvents.onGameSceneLoadRequested.Add(OnGameSceneLoadRequested);
	}

	private void Setup()
	{
		cb = base.gameObject.GetComponentUpwards<CelestialBody>();
		srfObj = SurfaceObject.Create(base.gameObject, cb, 5, KFSMUpdateMode.FIXEDUPDATE);
		setup = true;
	}

	private void OnGameSceneLoaded(GameScenes scn)
	{
		if (scn == GameScenes.SPACECENTER || scn == GameScenes.FLIGHT)
		{
			if (!setup)
			{
				Setup();
			}
			else
			{
				HighLogic.fetch.StartCoroutine(CallbackUtil.DelayedCallback(5, srfObj.PopToSceneRoot));
			}
		}
	}

	private void OnGameSceneLoadRequested(GameScenes scn)
	{
		GameScenes loadedScene = HighLogic.LoadedScene;
		if ((loadedScene == GameScenes.SPACECENTER || loadedScene == GameScenes.FLIGHT) && setup)
		{
			srfObj.ReturnToParent();
		}
	}

	private void OnDestroy()
	{
		if (setup && srfObj.IsPopped)
		{
			srfObj.ReturnToParent();
		}
		GameEvents.onLevelWasLoaded.Remove(OnGameSceneLoaded);
		GameEvents.onGameSceneLoadRequested.Remove(OnGameSceneLoadRequested);
	}
}

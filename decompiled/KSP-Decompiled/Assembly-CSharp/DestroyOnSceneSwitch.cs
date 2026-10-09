using UnityEngine;
using UnityEngine.SceneManagement;

public class DestroyOnSceneSwitch : MonoBehaviour
{
	private void Start()
	{
		GameEvents.onGameSceneLoadRequested.Add(OnSceneSwitch);
		SceneManager.sceneLoaded += OnSceneLoaded;
	}

	private void OnDestroy()
	{
		GameEvents.onGameSceneLoadRequested.Remove(OnSceneSwitch);
		SceneManager.sceneLoaded -= OnSceneLoaded;
	}

	private void OnSceneSwitch(GameScenes scene)
	{
		Object.DestroyImmediate(base.gameObject);
	}

	private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
	{
		OnLevelLoaded();
	}

	private void OnLevelLoaded()
	{
		Object.DestroyImmediate(base.gameObject);
	}
}

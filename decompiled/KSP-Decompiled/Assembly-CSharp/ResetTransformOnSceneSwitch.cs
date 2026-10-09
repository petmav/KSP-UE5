using UnityEngine;
using UnityEngine.SceneManagement;

public class ResetTransformOnSceneSwitch : MonoBehaviour
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
		base.transform.localRotation = Quaternion.identity;
		base.transform.localPosition = Vector3.zero;
	}

	private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
	{
		OnLevelLoaded();
	}

	private void OnLevelLoaded()
	{
		base.transform.localRotation = Quaternion.identity;
		base.transform.localPosition = Vector3.zero;
	}
}

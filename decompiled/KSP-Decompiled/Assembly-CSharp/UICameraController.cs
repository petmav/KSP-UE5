using KSP.UI;
using UnityEngine;

[ExecuteInEditMode]
public class UICameraController : MonoBehaviour
{
	private Camera cam;

	private int screenHeight = -1;

	private void Awake()
	{
		screenHeight = Screen.height;
		cam = GetComponent<Camera>();
		cam.orthographicSize = (float)screenHeight / 2f;
	}

	private void Start()
	{
		GameEvents.onGameSceneLoadRequested.Add(OnSceneChange);
		GameEvents.onScreenResolutionModified.Add(ScreenResolutionModified);
	}

	private void OnDestroy()
	{
		GameEvents.onGameSceneLoadRequested.Remove(OnSceneChange);
		GameEvents.onScreenResolutionModified.Remove(ScreenResolutionModified);
	}

	private void ScreenResolutionModified(int newWidth, int newHeight)
	{
		if (screenHeight != newHeight)
		{
			screenHeight = newHeight;
			cam.orthographicSize = (float)screenHeight * 0.5f;
		}
	}

	private void OnSceneChange(GameScenes scenes)
	{
		UIMasterController.Instance.uiCamera.enabled = true;
	}
}

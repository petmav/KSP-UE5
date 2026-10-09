using System.IO;
using UnityEngine;

public class ScreenShot : MonoBehaviour
{
	public Camera maincamera;

	private int i;

	private bool uiToggle;

	public bool allowUiHidingWithF2;

	public bool ScreenShotCameraMode;

	public bool useConfigSuperSize = true;

	public int superSize;

	private bool listenerAdded;

	private FlightCamera.TargetMode targetMode;

	private Transform target;

	private void Start()
	{
		i = 0;
		if (!Directory.Exists((Application.platform == RuntimePlatform.OSXPlayer) ? Path.Combine(Application.dataPath, "../../Screenshots") : Path.Combine(Application.dataPath, "../Screenshots")))
		{
			Directory.CreateDirectory((Application.platform == RuntimePlatform.OSXPlayer) ? Path.Combine(Application.dataPath, "../../Screenshots") : Path.Combine(Application.dataPath, "../Screenshots"));
		}
		if (allowUiHidingWithF2)
		{
			GameEvents.onShowUI.Add(ShowUI);
			GameEvents.onHideUI.Add(HideUI);
			listenerAdded = true;
		}
		if (useConfigSuperSize)
		{
			superSize = GameSettings.SCREENSHOT_SUPERSIZE;
		}
		GameEvents.onGameUnpause.Add(OnGameUnpause);
	}

	private void OnDestroy()
	{
		if (listenerAdded)
		{
			GameEvents.onShowUI.Remove(ShowUI);
			GameEvents.onHideUI.Remove(HideUI);
		}
		GameEvents.onGameUnpause.Remove(OnGameUnpause);
	}

	private void Update()
	{
		if (GameSettings.TAKE_SCREENSHOT.GetKeyDown())
		{
			i = 0;
			while (File.Exists(((Application.platform != RuntimePlatform.OSXPlayer) ? Path.Combine(Application.dataPath, "../Screenshots") : Path.Combine(Application.dataPath, "../../Screenshots")) + "/screenshot" + i + ".png"))
			{
				i++;
			}
			ScreenCapture.CaptureScreenshot(((Application.platform == RuntimePlatform.OSXPlayer) ? Path.Combine(Application.dataPath, "../../Screenshots") : Path.Combine(Application.dataPath, "../Screenshots")) + "/screenshot" + i + ".png", superSize);
			MonoBehaviour.print("SCREENSHOT!!");
		}
		if (!GameSettings.TOGGLE_UI.GetKeyDown() || !allowUiHidingWithF2 || HighLogic.LoadedScene != GameScenes.FLIGHT)
		{
			return;
		}
		if (uiToggle)
		{
			OnGameUnpause();
			GameEvents.onShowUI.Fire();
			return;
		}
		if (FlightDriver.Pause && FlightCamera.fetch != null)
		{
			ScreenShotCameraMode = true;
			targetMode = FlightCamera.fetch.targetMode;
			target = FlightCamera.fetch.Target;
			float distance = FlightCamera.fetch.Distance;
			FlightCamera.fetch.SetTarget(target, targetMode);
			FlightCamera.fetch.SetDistanceImmediate(distance);
		}
		GameEvents.onHideUI.Fire();
	}

	private void OnGameUnpause()
	{
		if (ScreenShotCameraMode)
		{
			ScreenShotCameraMode = false;
			if (FlightCamera.fetch != null)
			{
				FlightCamera.fetch.SetTarget(target, keepWorldPos: true, targetMode);
			}
		}
	}

	private void ShowUI()
	{
		uiToggle = false;
	}

	private void HideUI()
	{
		uiToggle = true;
	}
}

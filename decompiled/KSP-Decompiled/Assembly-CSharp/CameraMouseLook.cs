using UnityEngine;
using UnityEngine.SceneManagement;

public class CameraMouseLook : MonoBehaviour
{
	private static bool mouseLocked;

	public static bool MouseLocked => mouseLocked;

	public static bool GetMouseLook(bool arbitraryLock = false)
	{
		if (!GameSettings.CAMERA_MOUSE_TOGGLE.GetKeyDown() && (!GameSettings.CAMERA_DOUBLECLICK_MOUSELOOK || !Mouse.Right.GetDoubleClick()) && !(!mouseLocked && arbitraryLock))
		{
			if (mouseLocked)
			{
				if (Input.GetMouseButtonDown(1) || Mouse.Left.GetDoubleClick() || Mouse.Middle.GetDoubleClick() || Input.GetKeyDown(KeyCode.Escape))
				{
					SetMouseLook(mLock: false);
					if (Input.GetMouseButton(1))
					{
						return true;
					}
				}
				return mouseLocked;
			}
			return Input.GetMouseButton(1);
		}
		SetMouseLook(!mouseLocked);
		return mouseLocked;
	}

	public static void SetMouseLook(bool mLock)
	{
		if (mLock && !mouseLocked)
		{
			InputLockManager.SetControlLock(ControlTypes.ACTIONS_ALL | ControlTypes.EDITOR_PAD_PICK_PLACE | ControlTypes.EDITOR_PAD_PICK_COPY | ControlTypes.EDITOR_GIZMO_TOOLS | ControlTypes.EDITOR_ROOT_REFLOW | ControlTypes.KSC_FACILITIES | ControlTypes.GUI | ControlTypes.MAP_UI, "CameraMouseLook");
		}
		if (!mLock && mouseLocked)
		{
			InputLockManager.RemoveControlLock("CameraMouseLook");
		}
		Cursor.lockState = (mLock ? CursorLockMode.Locked : CursorLockMode.None);
		Cursor.visible = !mLock;
		mouseLocked = mLock;
	}

	protected void Awake()
	{
		GameEvents.onGamePause.Add(onGamePause);
		SceneManager.sceneLoaded += OnSceneLoaded;
	}

	protected void OnDestroy()
	{
		GameEvents.onGamePause.Remove(onGamePause);
		SceneManager.sceneLoaded -= OnSceneLoaded;
	}

	protected void onGamePause()
	{
		SetMouseLook(mLock: false);
	}

	private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
	{
		OnLevelLoaded(HighLogic.GetLoadedGameSceneFromBuildIndex(scene.buildIndex));
	}

	private void OnLevelLoaded(GameScenes lvl)
	{
		SetMouseLook(mLock: false);
	}
}

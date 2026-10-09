using System.Collections;
using KSP.UI;
using TMPro;
using UnityEngine;
using UnityEngine.SceneManagement;

public class LoadingBufferMask : MonoBehaviour
{
	public Camera camera;

	public GameObject loadingObject;

	public int loadingFrames = 75;

	[SerializeField]
	internal TextMeshPro textInfo;

	private bool isAsyncLoading;

	internal static LoadingBufferMask Instance;

	private void Start()
	{
		Instance = this;
		textInfo.text = "";
		GameEvents.onGameSceneLoadRequested.Add(OnSceneChange);
		SceneManager.sceneLoaded += OnSceneLoaded;
		Show();
	}

	private void OnDestroy()
	{
		GameEvents.onGameSceneLoadRequested.Remove(OnSceneChange);
		SceneManager.sceneLoaded -= OnSceneLoaded;
	}

	private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
	{
		OnLevelLoaded(HighLogic.GetLoadedGameSceneFromBuildIndex(scene.buildIndex));
	}

	private void OnLevelLoaded(GameScenes level)
	{
		switch (level)
		{
		case GameScenes.SPACECENTER:
			if (isAsyncLoading)
			{
				StartCoroutine(ShowDuration());
			}
			else
			{
				Hide();
			}
			break;
		case GameScenes.EDITOR:
			if (isAsyncLoading)
			{
				StartCoroutine(ShowDuration());
			}
			else
			{
				Hide();
			}
			break;
		case GameScenes.FLIGHT:
			StartCoroutine(ShowDuration());
			break;
		default:
			Hide();
			break;
		case GameScenes.LOADINGBUFFER:
		case GameScenes.PSYSTEM:
			Show();
			break;
		}
		if (level != GameScenes.LOADINGBUFFER)
		{
			isAsyncLoading = false;
		}
	}

	private void OnSceneChange(GameScenes scene)
	{
		StopCoroutine("ShowDuration");
		isAsyncLoading = HighLogic.fetch.sceneBufferTransitionMatrix.GetTransitionValue(HighLogic.LoadedScene, scene);
		Show();
	}

	internal void Show()
	{
		camera.enabled = true;
		loadingObject.SetActive(value: true);
		textInfo.gameObject.SetActive(value: true);
		UIMasterController.Instance.HideUI();
	}

	internal void Hide()
	{
		camera.enabled = false;
		loadingObject.SetActive(value: false);
		textInfo.gameObject.SetActive(value: false);
		UIMasterController.Instance.ShowUI();
	}

	private IEnumerator ShowDuration()
	{
		Show();
		int numFrames = 0;
		while (numFrames < loadingFrames)
		{
			int num = numFrames + 1;
			numFrames = num;
			yield return null;
		}
		if (HighLogic.LoadedSceneIsFlight && !FlightGlobals.ready)
		{
			yield return null;
		}
		while (HighLogic.LoadedSceneIsFlight && FlightGlobals.ActiveVessel != null && (!FlightGlobals.ActiveVessel.loaded || (FlightGlobals.ActiveVessel.packed && !((float)numFrames >= 400f))))
		{
			int num = numFrames + 1;
			numFrames = num;
			yield return null;
		}
		if (HighLogic.LoadedScene == GameScenes.SPACECENTER)
		{
			yield return new WaitForSeconds(2f);
		}
		Hide();
	}
}

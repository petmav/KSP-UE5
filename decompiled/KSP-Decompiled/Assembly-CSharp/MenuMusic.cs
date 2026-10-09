using UnityEngine;
using UnityEngine.SceneManagement;

public class MenuMusic : MonoBehaviour
{
	public AudioClip theme;

	public AudioClip ambienceLoop;

	private static bool themePlayedOnce;

	private static MenuMusic instance;

	private AudioSource _audioSource;

	private void Awake()
	{
		if ((bool)instance)
		{
			Object.Destroy(base.gameObject);
			return;
		}
		instance = this;
		this.GetComponentCached(ref _audioSource).volume = GameSettings.MUSIC_VOLUME;
		Object.DontDestroyOnLoad(base.gameObject);
		if (!themePlayedOnce)
		{
			PlayTheme();
			Invoke("PlayAmbienceLoop", theme.length);
			themePlayedOnce = true;
		}
		else
		{
			PlayAmbienceLoop();
		}
		SceneManager.sceneLoaded += OnSceneLoaded;
	}

	private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
	{
		OnLevelLoaded(HighLogic.GetLoadedGameSceneFromBuildIndex(scene.buildIndex));
	}

	private void OnLevelLoaded(GameScenes level)
	{
		if (level != GameScenes.SETTINGS && level != GameScenes.MAINMENU)
		{
			Object.Destroy(base.gameObject);
		}
	}

	private void OnDestroy()
	{
		SceneManager.sceneLoaded -= OnSceneLoaded;
		if (IsInvoking())
		{
			CancelInvoke();
		}
		if (instance != null && instance == this)
		{
			instance = null;
		}
	}

	private void PlayTheme()
	{
		this.GetComponentCached(ref _audioSource).clip = theme;
		this.GetComponentCached(ref _audioSource).loop = false;
		this.GetComponentCached(ref _audioSource).Play();
	}

	private void PlayAmbienceLoop()
	{
		this.GetComponentCached(ref _audioSource).clip = ambienceLoop;
		this.GetComponentCached(ref _audioSource).loop = true;
		this.GetComponentCached(ref _audioSource).Play();
	}
}

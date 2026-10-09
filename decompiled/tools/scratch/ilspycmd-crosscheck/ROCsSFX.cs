using UnityEngine;

[EffectDefinition("ROCSAUDIO")]
public class ROCsSFX : EffectBehaviour
{
	[KSPField]
	public string idleEffectName = "idle";

	[KSPField]
	public string burstEffectName = "burst";

	[Persistent]
	public bool loop;

	private AudioClip clipAudio;

	private AudioSource audioSource;

	private float soundMaxDistance = 500f;

	private bool isPaused;

	private bool onPauseWasPlaying;

	private float sfxPower;

	private bool fadeoutPlayback;

	private bool fadeInPlayback;

	private string clipPath;

	public float playDelay;

	public float fadeOutSpeed = 0.25f;

	public float fadeInSpeed = 0.25f;

	private void OnEnable()
	{
		GameEvents.onGamePause.Add(OnGamePause);
		GameEvents.onGameUnpause.Add(OnGameUnpause);
	}

	private void OnDisable()
	{
		GameEvents.onGamePause.Remove(OnGamePause);
		GameEvents.onGameUnpause.Remove(OnGameUnpause);
	}

	private void Update()
	{
		if (!(audioSource == null) && !isPaused)
		{
			if (fadeoutPlayback)
			{
				FadeOut();
			}
			if (fadeInPlayback)
			{
				FadeIn();
			}
		}
	}

	private void Start()
	{
		clipAudio = GameDatabase.Instance.GetAudioClip(clipPath);
		if (clipAudio == null)
		{
			Debug.Log("Cannot load AudioClip '" + clipPath + "' to AudioFX.");
		}
		if (effectName == idleEffectName)
		{
			PlayIdleSFX();
		}
	}

	private void Play(float power)
	{
		if (!(clipAudio == null))
		{
			if (audioSource == null)
			{
				CreateSource();
			}
			if (!audioSource.isPlaying)
			{
				audioSource.PlayDelayed(playDelay);
			}
		}
	}

	public void PlayIdleSFX()
	{
		Play(sfxPower);
	}

	public float PlayBurstSFX()
	{
		Play(sfxPower);
		if (clipAudio == null)
		{
			return 0f;
		}
		return clipAudio.length;
	}

	private void OnGamePause()
	{
		onPauseWasPlaying = false;
		if (audioSource != null)
		{
			onPauseWasPlaying = audioSource.isPlaying;
			audioSource.Pause();
		}
		isPaused = true;
	}

	private void OnGameUnpause()
	{
		if (audioSource != null && onPauseWasPlaying)
		{
			audioSource.Play();
		}
		isPaused = false;
	}

	public void FadeOutPlayback()
	{
		fadeoutPlayback = true;
	}

	private void FadeOut()
	{
		if (audioSource.volume > 0f)
		{
			audioSource.volume -= Time.deltaTime * fadeOutSpeed;
		}
		else
		{
			fadeoutPlayback = false;
		}
	}

	private void FadeIn()
	{
		if (audioSource.volume < sfxPower)
		{
			audioSource.volume += Time.deltaTime * fadeInSpeed;
		}
		else
		{
			fadeInPlayback = false;
		}
	}

	private void CreateSource()
	{
		AudioSource audioSource = base.gameObject.AddComponent<AudioSource>();
		audioSource.clip = clipAudio;
		audioSource.volume = sfxPower;
		audioSource.loop = loop;
		audioSource.dopplerLevel = 0f;
		audioSource.maxDistance = soundMaxDistance;
		audioSource.volume = sfxPower;
		audioSource.rolloffMode = AudioRolloffMode.Linear;
		audioSource.spatialBlend = 1f;
		this.audioSource = audioSource;
	}

	public void SetSFXVolume(float value)
	{
		sfxPower = value;
	}

	public void SetClipsPath(string idleClipPath, string burstClipPath)
	{
		if (effectName == idleEffectName)
		{
			clipPath = idleClipPath;
		}
		else if (effectName == burstEffectName)
		{
			clipPath = burstClipPath;
		}
		else
		{
			Debug.Log("effectName: " + effectName + ", has to match with the idle or burst state.");
		}
	}

	public void FadeInPlayback()
	{
		fadeInPlayback = true;
	}
}

using System.Collections.Generic;
using UnityEngine;

[EffectDefinition("AUDIO_SINGLE")]
public class AudioSingleFX : EffectBehaviour
{
	[Persistent]
	public AudioFX.AudioChannel channel;

	[Persistent]
	public int polyphony = 1;

	[Persistent]
	public string clip = "";

	public FXCurve volume = new FXCurve("volume", 1f);

	public FXCurve pitch = new FXCurve("pitch", 1f);

	private float volumeSet;

	private AudioClip clipAudio;

	private List<AudioSource> sources = new List<AudioSource>();

	private bool isPaused;

	private void Awake()
	{
		sources = new List<AudioSource>();
		AudioSource[] components = GetComponents<AudioSource>();
		int i = 0;
		for (int num = components.Length; i < num; i++)
		{
			Object.DestroyImmediate(components[i]);
		}
	}

	private void OnDestroy()
	{
		GameEvents.onGamePause.Remove(OnGamePause);
		GameEvents.onGameUnpause.Remove(OnGameUnpause);
	}

	private void Update()
	{
		if (sources.Count == 0 || isPaused)
		{
			return;
		}
		int count = sources.Count;
		while (count-- > 0)
		{
			AudioSource audioSource = sources[count];
			if (audioSource == null)
			{
				sources.RemoveAt(count);
			}
			else if (!audioSource.isPlaying)
			{
				Object.Destroy(audioSource);
				sources.RemoveAt(count);
			}
			else
			{
				AudioFX.SetSourceVolume(audioSource, volumeSet, channel);
			}
		}
	}

	public override void OnLoad(ConfigNode node)
	{
		ConfigNode.LoadObjectFromConfig(this, node);
		volume.Load("volume", node);
		pitch.Load("pitch", node);
	}

	public override void OnSave(ConfigNode node)
	{
		ConfigNode.CreateConfigFromObject(this, node);
		volume.Save(node);
		pitch.Save(node);
	}

	public override void OnInitialize()
	{
		clipAudio = GameDatabase.Instance.GetAudioClip(clip);
		if (clipAudio == null)
		{
			Debug.Log("Cannot assign AudioClip '" + clip + "' to AudioFX");
		}
		if (HighLogic.LoadedScene != 0)
		{
			GameEvents.onGamePause.Add(OnGamePause);
			GameEvents.onGameUnpause.Add(OnGameUnpause);
		}
	}

	public override void OnEvent(float power)
	{
		Play(power);
	}

	public override void OnEvent()
	{
		Play(1f);
	}

	private void Play(float power)
	{
		if (!(clipAudio == null))
		{
			AudioSource audioSource = null;
			audioSource = ((sources.Count >= polyphony) ? sources[0] : CreateSource());
			volumeSet = volume.Value(power);
			AudioFX.SetSourceVolume(audioSource, volumeSet, channel);
			audioSource.pitch = pitch.Value(power);
			audioSource.spatialBlend = 1f;
			if (!audioSource.isPlaying && !isPaused)
			{
				audioSource.Play();
			}
		}
	}

	private void Stop(AudioSource src)
	{
		int num = sources.IndexOf(src);
		if (num != -1)
		{
			Object.Destroy(sources[num]);
			sources.RemoveAt(num);
		}
	}

	private void Stop()
	{
		int count = sources.Count;
		while (count-- > 0)
		{
			Object.Destroy(sources[count]);
			sources.RemoveAt(count);
		}
	}

	private void OnGamePause()
	{
		isPaused = true;
		int count = sources.Count;
		while (count-- > 0)
		{
			sources[count].Pause();
		}
	}

	private void OnGameUnpause()
	{
		isPaused = false;
		int count = sources.Count;
		while (count-- > 0)
		{
			sources[count].Play();
		}
	}

	private AudioSource CreateSource()
	{
		AudioSource audioSource = base.gameObject.AddComponent<AudioSource>();
		audioSource.clip = clipAudio;
		audioSource.volume = volume;
		audioSource.pitch = pitch;
		audioSource.loop = false;
		sources.Add(audioSource);
		return audioSource;
	}
}

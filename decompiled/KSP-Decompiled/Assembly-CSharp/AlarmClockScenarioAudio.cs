using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Threading;
using UnityEngine;

public class AlarmClockScenarioAudio : MonoBehaviour
{
	internal delegate void AudioEventArgs(AlarmClockScenarioAudio sender, AudioClip clip);

	public class AudioClipMap
	{
		private readonly IDictionary<string, AudioClip> _clips = new Dictionary<string, AudioClip>();

		public AudioClip this[string key]
		{
			get
			{
				if (_clips.ContainsKey(key))
				{
					return _clips[key];
				}
				AudioClip value = LoadAudioClip(key);
				_clips[key] = value;
				return _clips[key];
			}
		}
	}

	private AudioClipMap alarmSounds;

	private AudioSource audioSource;

	public const string soundBaseUrl = "Squad/Alarms/Sounds/";

	[CompilerGenerated]
	private AudioEventArgs onPlayFinished;

	[CompilerGenerated]
	private AudioEventArgs onPlayStarted;

	private bool playing;

	private int repeatCounter;

	private int repeatLimit;

	public AudioClipMap AlarmSounds => alarmSounds;

	internal bool isPlaying => playing;

	internal event AudioEventArgs _001E
	{
		[CompilerGenerated]
		add
		{
			AudioEventArgs audioEventArgs = onPlayFinished;
			AudioEventArgs audioEventArgs2;
			do
			{
				audioEventArgs2 = audioEventArgs;
				AudioEventArgs value2 = (AudioEventArgs)Delegate.Combine(audioEventArgs2, value);
				audioEventArgs = Interlocked.CompareExchange(ref onPlayFinished, value2, audioEventArgs2);
			}
			while ((object)audioEventArgs != audioEventArgs2);
		}
		[CompilerGenerated]
		remove
		{
			AudioEventArgs audioEventArgs = onPlayFinished;
			AudioEventArgs audioEventArgs2;
			do
			{
				audioEventArgs2 = audioEventArgs;
				AudioEventArgs value2 = (AudioEventArgs)Delegate.Remove(audioEventArgs2, value);
				audioEventArgs = Interlocked.CompareExchange(ref onPlayFinished, value2, audioEventArgs2);
			}
			while ((object)audioEventArgs != audioEventArgs2);
		}
	}

	internal event AudioEventArgs _0003
	{
		[CompilerGenerated]
		add
		{
			AudioEventArgs audioEventArgs = onPlayStarted;
			AudioEventArgs audioEventArgs2;
			do
			{
				audioEventArgs2 = audioEventArgs;
				AudioEventArgs value2 = (AudioEventArgs)Delegate.Combine(audioEventArgs2, value);
				audioEventArgs = Interlocked.CompareExchange(ref onPlayStarted, value2, audioEventArgs2);
			}
			while ((object)audioEventArgs != audioEventArgs2);
		}
		[CompilerGenerated]
		remove
		{
			AudioEventArgs audioEventArgs = onPlayStarted;
			AudioEventArgs audioEventArgs2;
			do
			{
				audioEventArgs2 = audioEventArgs;
				AudioEventArgs value2 = (AudioEventArgs)Delegate.Remove(audioEventArgs2, value);
				audioEventArgs = Interlocked.CompareExchange(ref onPlayStarted, value2, audioEventArgs2);
			}
			while ((object)audioEventArgs != audioEventArgs2);
		}
	}

	private void Awake()
	{
		alarmSounds = new AudioClipMap();
		InitializeSoundController();
	}

	private void Start()
	{
	}

	private void InitializeSoundController()
	{
		if (audioSource == null)
		{
			audioSource = base.gameObject.AddComponent<AudioSource>();
			audioSource.spatialBlend = 0f;
			audioSource.playOnAwake = false;
			audioSource.loop = false;
			audioSource.Stop();
		}
	}

	public List<string> GetStockAlarmSounds()
	{
		List<string> list = new List<string>();
		for (int i = 0; i < GameDatabase.Instance.databaseAudio.Count; i++)
		{
			if (GameDatabase.Instance.databaseAudio[i] != null && GameDatabase.Instance.databaseAudio[i].name.StartsWith("Squad/Alarms/Sounds/"))
			{
				list.Add(GameDatabase.Instance.databaseAudio[i].name.Replace("Squad/Alarms/Sounds/", ""));
			}
		}
		return list;
	}

	internal bool PlaySound(string soundURL, int repeats = 1)
	{
		AudioClip audioClip = AlarmSounds[soundURL];
		if (audioClip == null)
		{
			Debug.LogWarning("[AlarmClockScenarioAudio]: Scenario unable to find sound " + soundURL + " - unable to play sound.");
			return false;
		}
		audioSource.clip = audioClip;
		audioSource.loop = false;
		audioSource.volume = GameSettings.UI_VOLUME;
		repeatCounter = 0;
		repeatLimit = repeats;
		playing = true;
		audioSource.Play();
		if (onPlayStarted != null)
		{
			onPlayStarted(this, audioClip);
		}
		return true;
	}

	internal void Stop()
	{
		audioSource.Stop();
		playing = false;
		if (onPlayFinished != null)
		{
			onPlayFinished(this, audioSource.clip);
		}
	}

	internal bool isClipPlaying(AudioClip clip)
	{
		if (playing)
		{
			return clip == audioSource.clip;
		}
		return false;
	}

	private void Update()
	{
		if (audioSource.isPlaying || !playing)
		{
			return;
		}
		repeatCounter++;
		if (repeatCounter < repeatLimit)
		{
			audioSource.Play();
			return;
		}
		playing = false;
		if (onPlayFinished != null)
		{
			onPlayFinished(this, audioSource.clip);
		}
	}

	public static AudioClip LoadAudioClip(string soundName)
	{
		soundName = (soundName.Contains("/") ? soundName : ("Squad/Alarms/Sounds/" + soundName));
		AudioClip audioClip = GameDatabase.Instance.GetAudioClip(soundName);
		_ = audioClip == null;
		return audioClip;
	}
}

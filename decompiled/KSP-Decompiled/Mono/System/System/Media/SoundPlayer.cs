using System.ComponentModel;
using System.IO;
using System.Net;
using System.Runtime.Serialization;
using System.Threading;
using Mono.Audio;

namespace System.Media;

[Serializable]
[ToolboxItem(false)]
public class SoundPlayer : Component, ISerializable
{
	private string sound_location;

	private Stream audiostream;

	private object tag = string.Empty;

	private MemoryStream mstream;

	private bool load_completed;

	private int load_timeout = 10000;

	private AudioDevice adev;

	private AudioData adata;

	private bool stopped;

	private Win32SoundPlayer win32_player;

	private static readonly bool use_win32_player;

	public bool IsLoadCompleted => load_completed;

	public int LoadTimeout
	{
		get
		{
			return load_timeout;
		}
		set
		{
			if (value < 0)
			{
				throw new ArgumentException("timeout must be >= 0");
			}
			load_timeout = value;
		}
	}

	public string SoundLocation
	{
		get
		{
			return sound_location;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			sound_location = value;
			load_completed = false;
			OnSoundLocationChanged(EventArgs.Empty);
			if (this.SoundLocationChanged != null)
			{
				this.SoundLocationChanged(this, EventArgs.Empty);
			}
		}
	}

	public Stream Stream
	{
		get
		{
			return audiostream;
		}
		set
		{
			if (audiostream != value)
			{
				audiostream = value;
				load_completed = false;
				OnStreamChanged(EventArgs.Empty);
				if (this.StreamChanged != null)
				{
					this.StreamChanged(this, EventArgs.Empty);
				}
			}
		}
	}

	public object Tag
	{
		get
		{
			return tag;
		}
		set
		{
			tag = value;
		}
	}

	public event AsyncCompletedEventHandler LoadCompleted;

	public event EventHandler SoundLocationChanged;

	public event EventHandler StreamChanged;

	static SoundPlayer()
	{
		use_win32_player = Environment.OSVersion.Platform != PlatformID.Unix;
	}

	public SoundPlayer()
	{
		sound_location = string.Empty;
	}

	public SoundPlayer(Stream stream)
		: this()
	{
		audiostream = stream;
	}

	public SoundPlayer(string soundLocation)
		: this()
	{
		if (soundLocation == null)
		{
			throw new ArgumentNullException("soundLocation");
		}
		sound_location = soundLocation;
	}

	protected SoundPlayer(SerializationInfo serializationInfo, StreamingContext context)
		: this()
	{
		throw new NotImplementedException();
	}

	private void LoadFromStream(Stream s)
	{
		mstream = new MemoryStream();
		byte[] buffer = new byte[4096];
		int count;
		while ((count = s.Read(buffer, 0, 4096)) > 0)
		{
			mstream.Write(buffer, 0, count);
		}
		mstream.Position = 0L;
	}

	private void LoadFromUri(string location)
	{
		mstream = null;
		Stream stream = null;
		if (string.IsNullOrEmpty(location))
		{
			return;
		}
		stream = ((!File.Exists(location)) ? WebRequest.Create(location).GetResponse().GetResponseStream() : new FileStream(location, FileMode.Open, FileAccess.Read, FileShare.Read));
		using (stream)
		{
			LoadFromStream(stream);
		}
	}

	public void Load()
	{
		if (load_completed)
		{
			return;
		}
		if (audiostream != null)
		{
			LoadFromStream(audiostream);
		}
		else
		{
			LoadFromUri(sound_location);
		}
		adata = null;
		adev = null;
		load_completed = true;
		AsyncCompletedEventArgs e = new AsyncCompletedEventArgs(null, cancelled: false, this);
		OnLoadCompleted(e);
		if (this.LoadCompleted != null)
		{
			this.LoadCompleted(this, e);
		}
		if (use_win32_player)
		{
			if (win32_player == null)
			{
				win32_player = new Win32SoundPlayer(mstream);
			}
			else
			{
				win32_player.Stream = mstream;
			}
		}
	}

	private void AsyncFinished(IAsyncResult ar)
	{
		(ar.AsyncState as ThreadStart).EndInvoke(ar);
	}

	public void LoadAsync()
	{
		if (!load_completed)
		{
			ThreadStart threadStart = Load;
			threadStart.BeginInvoke(AsyncFinished, threadStart);
		}
	}

	protected virtual void OnLoadCompleted(AsyncCompletedEventArgs e)
	{
	}

	protected virtual void OnSoundLocationChanged(EventArgs e)
	{
	}

	protected virtual void OnStreamChanged(EventArgs e)
	{
	}

	private void Start()
	{
		if (!use_win32_player)
		{
			stopped = false;
			if (adata != null)
			{
				adata.IsStopped = false;
			}
		}
		if (!load_completed)
		{
			Load();
		}
	}

	public void Play()
	{
		if (!use_win32_player)
		{
			ThreadStart threadStart = PlaySync;
			threadStart.BeginInvoke(AsyncFinished, threadStart);
			return;
		}
		Start();
		if (mstream == null)
		{
			SystemSounds.Beep.Play();
		}
		else
		{
			win32_player.Play();
		}
	}

	private void PlayLoop()
	{
		Start();
		if (mstream == null)
		{
			SystemSounds.Beep.Play();
			return;
		}
		while (!stopped)
		{
			PlaySync();
		}
	}

	public void PlayLooping()
	{
		if (!use_win32_player)
		{
			ThreadStart threadStart = PlayLoop;
			threadStart.BeginInvoke(AsyncFinished, threadStart);
			return;
		}
		Start();
		if (mstream == null)
		{
			SystemSounds.Beep.Play();
		}
		else
		{
			win32_player.PlayLooping();
		}
	}

	public void PlaySync()
	{
		Start();
		if (mstream == null)
		{
			SystemSounds.Beep.Play();
			return;
		}
		if (!use_win32_player)
		{
			try
			{
				if (adata == null)
				{
					adata = new WavData(mstream);
				}
				if (adev == null)
				{
					adev = AudioDevice.CreateDevice(null);
				}
				if (adata != null)
				{
					adata.Setup(adev);
					adata.Play(adev);
				}
				return;
			}
			catch
			{
				return;
			}
		}
		win32_player.PlaySync();
	}

	public void Stop()
	{
		if (!use_win32_player)
		{
			stopped = true;
			if (adata != null)
			{
				adata.IsStopped = true;
			}
		}
		else
		{
			win32_player.Stop();
		}
	}

	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
	}
}

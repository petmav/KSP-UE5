using System.ComponentModel;
using System.IO;
using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Media;

[Serializable]
[ToolboxItem(false)]
[HostProtection(SecurityAction.LinkDemand, UI = true)]
public class SoundPlayer : Component, ISerializable
{
	public bool IsLoadCompleted
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public int LoadTimeout
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string SoundLocation
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Stream Stream
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public object Tag
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event AsyncCompletedEventHandler LoadCompleted
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event EventHandler SoundLocationChanged
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event EventHandler StreamChanged
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public SoundPlayer()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SoundPlayer(Stream stream)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected SoundPlayer(SerializationInfo serializationInfo, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SoundPlayer(string soundLocation)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Load()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void LoadAsync()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnLoadCompleted(AsyncCompletedEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnSoundLocationChanged(EventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnStreamChanged(EventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Play()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void PlayLooping()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void PlaySync()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Stop()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.SerializationFormatter)]
	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

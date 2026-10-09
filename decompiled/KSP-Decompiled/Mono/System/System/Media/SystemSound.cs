using System.IO;
using Unity;

namespace System.Media;

public class SystemSound
{
	private Stream resource;

	internal SystemSound(string tag)
	{
		resource = typeof(SystemSound).Assembly.GetManifestResourceStream(tag + ".wav");
	}

	public void Play()
	{
		new SoundPlayer(resource).Play();
	}

	internal SystemSound()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

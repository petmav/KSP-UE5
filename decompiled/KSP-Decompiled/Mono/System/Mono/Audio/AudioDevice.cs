namespace Mono.Audio;

internal class AudioDevice
{
	protected uint chunk_size;

	public uint ChunkSize => chunk_size;

	private static AudioDevice TryAlsa(string name)
	{
		try
		{
			return new AlsaDevice(name);
		}
		catch
		{
			return null;
		}
	}

	public static AudioDevice CreateDevice(string name)
	{
		AudioDevice audioDevice = TryAlsa(name);
		if (audioDevice == null)
		{
			audioDevice = new AudioDevice();
		}
		return audioDevice;
	}

	public virtual bool SetFormat(AudioFormat format, int channels, int rate)
	{
		return true;
	}

	public virtual int PlaySample(byte[] buffer, int num_frames)
	{
		return num_frames;
	}

	public virtual int XRunRecovery(int err)
	{
		return err;
	}

	public virtual void Wait()
	{
	}
}

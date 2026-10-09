using System;
using System.IO;
using System.Runtime.InteropServices;

namespace Mono.Audio;

internal class Win32SoundPlayer : IDisposable
{
	private enum SoundFlags : uint
	{
		SND_SYNC = 0u,
		SND_ASYNC = 1u,
		SND_NODEFAULT = 2u,
		SND_MEMORY = 4u,
		SND_LOOP = 8u,
		SND_FILENAME = 0x20000u
	}

	private byte[] _buffer;

	private bool _disposed;

	public Stream Stream
	{
		set
		{
			Stop();
			if (value != null)
			{
				_buffer = new byte[value.Length];
				value.Read(_buffer, 0, _buffer.Length);
			}
			else
			{
				_buffer = new byte[0];
			}
		}
	}

	public Win32SoundPlayer(Stream s)
	{
		if (s != null)
		{
			_buffer = new byte[s.Length];
			s.Read(_buffer, 0, _buffer.Length);
		}
		else
		{
			_buffer = new byte[0];
		}
	}

	[DllImport("winmm.dll", SetLastError = true)]
	private static extern bool PlaySound(byte[] ptrToSound, UIntPtr hmod, SoundFlags flags);

	public void Dispose()
	{
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	~Win32SoundPlayer()
	{
		Dispose(disposing: false);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (!_disposed)
		{
			Stop();
			_disposed = true;
		}
	}

	public void Play()
	{
		PlaySound(_buffer, UIntPtr.Zero, (SoundFlags)5u);
	}

	public void PlayLooping()
	{
		PlaySound(_buffer, UIntPtr.Zero, (SoundFlags)13u);
	}

	public void PlaySync()
	{
		PlaySound(_buffer, UIntPtr.Zero, (SoundFlags)6u);
	}

	public void Stop()
	{
		PlaySound(null, UIntPtr.Zero, SoundFlags.SND_SYNC);
	}
}

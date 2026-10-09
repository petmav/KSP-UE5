using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace System.Security.Cryptography;

[ComVisible(true)]
public sealed class RNGCryptoServiceProvider : RandomNumberGenerator
{
	private static object _lock;

	private IntPtr _handle;

	static RNGCryptoServiceProvider()
	{
		if (RngOpen())
		{
			_lock = new object();
		}
	}

	public RNGCryptoServiceProvider()
	{
		_handle = RngInitialize(null);
		Check();
	}

	public RNGCryptoServiceProvider(byte[] rgb)
	{
		_handle = RngInitialize(rgb);
		Check();
	}

	public RNGCryptoServiceProvider(CspParameters cspParams)
	{
		_handle = RngInitialize(null);
		Check();
	}

	public RNGCryptoServiceProvider(string str)
	{
		if (str == null)
		{
			_handle = RngInitialize(null);
		}
		else
		{
			_handle = RngInitialize(Encoding.UTF8.GetBytes(str));
		}
		Check();
	}

	private void Check()
	{
		if (_handle == IntPtr.Zero)
		{
			throw new CryptographicException(Locale.GetText("Couldn't access random source."));
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern bool RngOpen();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr RngInitialize(byte[] seed);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr RngGetBytes(IntPtr handle, byte[] data);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void RngClose(IntPtr handle);

	public override void GetBytes(byte[] data)
	{
		if (data == null)
		{
			throw new ArgumentNullException("data");
		}
		if (_lock == null)
		{
			_handle = RngGetBytes(_handle, data);
		}
		else
		{
			lock (_lock)
			{
				_handle = RngGetBytes(_handle, data);
			}
		}
		Check();
	}

	public override void GetNonZeroBytes(byte[] data)
	{
		if (data == null)
		{
			throw new ArgumentNullException("data");
		}
		byte[] array = new byte[data.Length * 2];
		int num = 0;
		while (num < data.Length)
		{
			_handle = RngGetBytes(_handle, array);
			Check();
			for (int i = 0; i < array.Length; i++)
			{
				if (num == data.Length)
				{
					break;
				}
				if (array[i] != 0)
				{
					data[num++] = array[i];
				}
			}
		}
	}

	~RNGCryptoServiceProvider()
	{
		if (_handle != IntPtr.Zero)
		{
			RngClose(_handle);
			_handle = IntPtr.Zero;
		}
	}

	protected override void Dispose(bool disposing)
	{
		base.Dispose(disposing);
	}
}

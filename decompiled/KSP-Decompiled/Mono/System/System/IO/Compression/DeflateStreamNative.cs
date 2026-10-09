using System.Runtime.InteropServices;
using Mono.Util;

namespace System.IO.Compression;

internal class DeflateStreamNative
{
	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	private delegate int UnmanagedReadOrWrite(IntPtr buffer, int length, IntPtr data);

	private sealed class SafeDeflateStreamHandle : SafeHandle
	{
		public override bool IsInvalid => handle == IntPtr.Zero;

		private SafeDeflateStreamHandle()
			: base(IntPtr.Zero, ownsHandle: true)
		{
		}

		protected override bool ReleaseHandle()
		{
			CloseZStream(handle);
			return true;
		}
	}

	private const int BufferSize = 4096;

	private UnmanagedReadOrWrite feeder;

	private Stream base_stream;

	private SafeDeflateStreamHandle z_stream;

	private GCHandle data;

	private bool disposed;

	private byte[] io_buffer;

	private const string LIBNAME = "MonoPosixHelper";

	private DeflateStreamNative()
	{
	}

	public static DeflateStreamNative Create(Stream compressedStream, CompressionMode mode, bool gzip)
	{
		DeflateStreamNative deflateStreamNative = new DeflateStreamNative();
		deflateStreamNative.data = GCHandle.Alloc(deflateStreamNative);
		deflateStreamNative.feeder = ((mode == CompressionMode.Compress) ? new UnmanagedReadOrWrite(UnmanagedWrite) : new UnmanagedReadOrWrite(UnmanagedRead));
		deflateStreamNative.z_stream = CreateZStream(mode, gzip, deflateStreamNative.feeder, GCHandle.ToIntPtr(deflateStreamNative.data));
		if (deflateStreamNative.z_stream.IsInvalid)
		{
			deflateStreamNative.Dispose(disposing: true);
			return null;
		}
		deflateStreamNative.base_stream = compressedStream;
		return deflateStreamNative;
	}

	~DeflateStreamNative()
	{
		Dispose(disposing: false);
	}

	public void Dispose(bool disposing)
	{
		if (disposing && !disposed)
		{
			disposed = true;
			GC.SuppressFinalize(this);
			io_buffer = null;
			z_stream.Dispose();
		}
		if (data.IsAllocated)
		{
			data.Free();
		}
	}

	public void Flush()
	{
		CheckResult(Flush(z_stream), "Flush");
	}

	public int ReadZStream(IntPtr buffer, int length)
	{
		int result = ReadZStream(z_stream, buffer, length);
		CheckResult(result, "ReadInternal");
		return result;
	}

	public void WriteZStream(IntPtr buffer, int length)
	{
		CheckResult(WriteZStream(z_stream, buffer, length), "WriteInternal");
	}

	[MonoPInvokeCallback(typeof(UnmanagedReadOrWrite))]
	private static int UnmanagedRead(IntPtr buffer, int length, IntPtr data)
	{
		if (!(GCHandle.FromIntPtr(data).Target is DeflateStreamNative deflateStreamNative))
		{
			return -1;
		}
		return deflateStreamNative.UnmanagedRead(buffer, length);
	}

	private int UnmanagedRead(IntPtr buffer, int length)
	{
		if (io_buffer == null)
		{
			io_buffer = new byte[4096];
		}
		int count = Math.Min(length, io_buffer.Length);
		int num = base_stream.Read(io_buffer, 0, count);
		if (num > 0)
		{
			Marshal.Copy(io_buffer, 0, buffer, num);
		}
		return num;
	}

	[MonoPInvokeCallback(typeof(UnmanagedReadOrWrite))]
	private static int UnmanagedWrite(IntPtr buffer, int length, IntPtr data)
	{
		if (!(GCHandle.FromIntPtr(data).Target is DeflateStreamNative deflateStreamNative))
		{
			return -1;
		}
		return deflateStreamNative.UnmanagedWrite(buffer, length);
	}

	private unsafe int UnmanagedWrite(IntPtr buffer, int length)
	{
		int num = 0;
		while (length > 0)
		{
			if (io_buffer == null)
			{
				io_buffer = new byte[4096];
			}
			int num2 = Math.Min(length, io_buffer.Length);
			Marshal.Copy(buffer, io_buffer, 0, num2);
			base_stream.Write(io_buffer, 0, num2);
			buffer = new IntPtr((byte*)buffer.ToPointer() + num2);
			length -= num2;
			num += num2;
		}
		return num;
	}

	private static void CheckResult(int result, string where)
	{
		if (result >= 0)
		{
			return;
		}
		throw new IOException(result switch
		{
			-1 => "Unknown error", 
			-2 => "Internal error", 
			-3 => "Corrupted data", 
			-4 => "Not enough memory", 
			-5 => "Internal error (no progress possible)", 
			-6 => "Invalid version", 
			-10 => "Invalid argument(s)", 
			-11 => "IO error", 
			_ => "Unknown error", 
		} + " " + where);
	}

	[DllImport("MonoPosixHelper", CallingConvention = CallingConvention.Cdecl)]
	private static extern SafeDeflateStreamHandle CreateZStream(CompressionMode compress, bool gzip, UnmanagedReadOrWrite feeder, IntPtr data);

	[DllImport("MonoPosixHelper", CallingConvention = CallingConvention.Cdecl)]
	private static extern int CloseZStream(IntPtr stream);

	[DllImport("MonoPosixHelper", CallingConvention = CallingConvention.Cdecl)]
	private static extern int Flush(SafeDeflateStreamHandle stream);

	[DllImport("MonoPosixHelper", CallingConvention = CallingConvention.Cdecl)]
	private static extern int ReadZStream(SafeDeflateStreamHandle stream, IntPtr buffer, int length);

	[DllImport("MonoPosixHelper", CallingConvention = CallingConvention.Cdecl)]
	private static extern int WriteZStream(SafeDeflateStreamHandle stream, IntPtr buffer, int length);
}

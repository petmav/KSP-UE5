using System.Runtime.ConstrainedExecution;
using Microsoft.Win32.SafeHandles;

namespace System.Runtime.InteropServices;

public abstract class SafeBuffer : SafeHandleZeroOrMinusOneIsInvalid, IDisposable
{
	private ulong byte_length;

	private unsafe byte* last_byte;

	private bool inited;

	[CLSCompliant(false)]
	public ulong ByteLength => byte_length;

	protected SafeBuffer(bool ownsHandle)
		: base(ownsHandle)
	{
	}

	[CLSCompliant(false)]
	public unsafe void Initialize(ulong numBytes)
	{
		if (numBytes == 0L)
		{
			throw new ArgumentOutOfRangeException("numBytes");
		}
		inited = true;
		byte_length = numBytes;
		last_byte = (byte*)(void*)handle + numBytes;
	}

	[CLSCompliant(false)]
	public void Initialize(uint numElements, uint sizeOfEachElement)
	{
		Initialize(numElements * sizeOfEachElement);
	}

	[CLSCompliant(false)]
	public void Initialize<T>(uint numElements) where T : struct
	{
		Initialize(numElements, (uint)Marshal.SizeOf(typeof(T)));
	}

	[CLSCompliant(false)]
	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public unsafe void AcquirePointer(ref byte* pointer)
	{
		if (!inited)
		{
			throw new InvalidOperationException();
		}
		bool success = false;
		DangerousAddRef(ref success);
		if (success)
		{
			pointer = (byte*)(void*)handle;
		}
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
	public void ReleasePointer()
	{
		if (!inited)
		{
			throw new InvalidOperationException();
		}
		DangerousRelease();
	}

	[CLSCompliant(false)]
	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public unsafe T Read<T>(ulong byteOffset) where T : struct
	{
		if (!inited)
		{
			throw new InvalidOperationException();
		}
		byte* ptr = (byte*)(void*)handle + byteOffset;
		if (ptr >= last_byte || ptr + Marshal.SizeOf(typeof(T)) > last_byte)
		{
			throw new ArgumentException("byteOffset");
		}
		return (T)Marshal.PtrToStructure((IntPtr)ptr, typeof(T));
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	[CLSCompliant(false)]
	public unsafe void ReadArray<T>(ulong byteOffset, T[] array, int index, int count) where T : struct
	{
		if (!inited)
		{
			throw new InvalidOperationException();
		}
		int num = Marshal.SizeOf(typeof(T)) * count;
		byte* ptr = (byte*)(void*)handle + byteOffset;
		if (ptr >= last_byte || ptr + num > last_byte)
		{
			throw new ArgumentException("byteOffset");
		}
		Marshal.copy_from_unmanaged((IntPtr)ptr, index, array, count);
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	[CLSCompliant(false)]
	public unsafe void Write<T>(ulong byteOffset, T value) where T : struct
	{
		if (!inited)
		{
			throw new InvalidOperationException();
		}
		byte* ptr = (byte*)(void*)handle + byteOffset;
		if (ptr >= last_byte || ptr + Marshal.SizeOf(typeof(T)) > last_byte)
		{
			throw new ArgumentException("byteOffset");
		}
		Marshal.StructureToPtr(value, (IntPtr)ptr, fDeleteOld: false);
	}

	[CLSCompliant(false)]
	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public unsafe void WriteArray<T>(ulong byteOffset, T[] array, int index, int count) where T : struct
	{
		if (!inited)
		{
			throw new InvalidOperationException();
		}
		byte* ptr = (byte*)(void*)handle + byteOffset;
		int num = Marshal.SizeOf(typeof(T)) * count;
		if (ptr >= last_byte || ptr + num > last_byte)
		{
			throw new ArgumentException("would overrite");
		}
		Marshal.copy_to_unmanaged(array, index, (IntPtr)ptr, count);
	}
}

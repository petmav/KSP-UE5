using System.Runtime.CompilerServices;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
using System.Security;

namespace System;

[ComVisible(true)]
public static class Buffer
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecuritySafeCritical]
	internal static extern bool InternalBlockCopy(Array src, int srcOffsetBytes, Array dst, int dstOffsetBytes, int byteCount);

	[SecurityCritical]
	internal unsafe static int IndexOfByte(byte* src, byte value, int index, int count)
	{
		byte* ptr;
		for (ptr = src + index; ((uint)(int)ptr & 3u) != 0; ptr++)
		{
			if (count == 0)
			{
				return -1;
			}
			if (*ptr == value)
			{
				return (int)(ptr - src);
			}
			count--;
		}
		uint num = (uint)((value << 8) + value);
		num = (num << 16) + num;
		while (count > 3)
		{
			uint num2 = *(uint*)ptr;
			num2 ^= num;
			uint num3 = 2130640639 + num2;
			num2 ^= 0xFFFFFFFFu;
			num2 ^= num3;
			if ((num2 & 0x81010100u) != 0)
			{
				int num4 = (int)(ptr - src);
				if (*ptr == value)
				{
					return num4;
				}
				if (ptr[1] == value)
				{
					return num4 + 1;
				}
				if (ptr[2] == value)
				{
					return num4 + 2;
				}
				if (ptr[3] == value)
				{
					return num4 + 3;
				}
			}
			count -= 4;
			ptr += 4;
		}
		while (count > 0)
		{
			if (*ptr == value)
			{
				return (int)(ptr - src);
			}
			count--;
			ptr++;
		}
		return -1;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecurityCritical]
	private static extern byte _GetByte(Array array, int index);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecurityCritical]
	private static extern void _SetByte(Array array, int index, byte value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecurityCritical]
	private static extern int _ByteLength(Array array);

	[SecurityCritical]
	internal unsafe static void ZeroMemory(byte* src, long len)
	{
		while (len-- > 0)
		{
			src[len] = 0;
		}
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
	[SecurityCritical]
	internal unsafe static void Memcpy(byte[] dest, int destIndex, byte* src, int srcIndex, int len)
	{
		if (len != 0)
		{
			fixed (byte* ptr = dest)
			{
				Memcpy(ptr + destIndex, src + srcIndex, len);
			}
		}
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
	[SecurityCritical]
	internal unsafe static void Memcpy(byte* pDest, int destIndex, byte[] src, int srcIndex, int len)
	{
		if (len != 0)
		{
			fixed (byte* ptr = src)
			{
				Memcpy(pDest + destIndex, ptr + srcIndex, len);
			}
		}
	}

	public static int ByteLength(Array array)
	{
		if (array == null)
		{
			throw new ArgumentNullException("array");
		}
		int num = _ByteLength(array);
		if (num < 0)
		{
			throw new ArgumentException(Locale.GetText("Object must be an array of primitives."));
		}
		return num;
	}

	public static byte GetByte(Array array, int index)
	{
		if (index < 0 || index >= ByteLength(array))
		{
			throw new ArgumentOutOfRangeException("index");
		}
		return _GetByte(array, index);
	}

	public static void SetByte(Array array, int index, byte value)
	{
		if (index < 0 || index >= ByteLength(array))
		{
			throw new ArgumentOutOfRangeException("index");
		}
		_SetByte(array, index, value);
	}

	public static void BlockCopy(Array src, int srcOffset, Array dst, int dstOffset, int count)
	{
		if (src == null)
		{
			throw new ArgumentNullException("src");
		}
		if (dst == null)
		{
			throw new ArgumentNullException("dst");
		}
		if (srcOffset < 0)
		{
			throw new ArgumentOutOfRangeException("srcOffset", Locale.GetText("Non-negative number required."));
		}
		if (dstOffset < 0)
		{
			throw new ArgumentOutOfRangeException("dstOffset", Locale.GetText("Non-negative number required."));
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Locale.GetText("Non-negative number required."));
		}
		if (!InternalBlockCopy(src, srcOffset, dst, dstOffset, count) && (srcOffset > ByteLength(src) - count || dstOffset > ByteLength(dst) - count))
		{
			throw new ArgumentException(Locale.GetText("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection."));
		}
	}

	[CLSCompliant(false)]
	public unsafe static void MemoryCopy(void* source, void* destination, long destinationSizeInBytes, long sourceBytesToCopy)
	{
		if (sourceBytesToCopy > destinationSizeInBytes)
		{
			ThrowHelper.ThrowArgumentOutOfRangeException(ExceptionArgument.sourceBytesToCopy);
		}
		byte* ptr = (byte*)source;
		byte* ptr2 = (byte*)destination;
		while (sourceBytesToCopy > int.MaxValue)
		{
			Memcpy(ptr2, ptr, int.MaxValue);
			sourceBytesToCopy -= int.MaxValue;
			ptr += int.MaxValue;
			ptr2 += int.MaxValue;
		}
		memcpy1(ptr2, ptr, (int)sourceBytesToCopy);
	}

	[CLSCompliant(false)]
	public unsafe static void MemoryCopy(void* source, void* destination, ulong destinationSizeInBytes, ulong sourceBytesToCopy)
	{
		if (sourceBytesToCopy > destinationSizeInBytes)
		{
			ThrowHelper.ThrowArgumentOutOfRangeException(ExceptionArgument.sourceBytesToCopy);
		}
		byte* ptr = (byte*)source;
		byte* ptr2 = (byte*)destination;
		while (sourceBytesToCopy > int.MaxValue)
		{
			Memcpy(ptr2, ptr, int.MaxValue);
			sourceBytesToCopy -= int.MaxValue;
			ptr += int.MaxValue;
			ptr2 += int.MaxValue;
		}
		Memcpy(ptr2, ptr, (int)sourceBytesToCopy);
	}

	internal unsafe static void memcpy4(byte* dest, byte* src, int size)
	{
		while (size >= 16)
		{
			*(int*)dest = *(int*)src;
			*(int*)(dest + 4) = *(int*)(src + 4);
			*(int*)(dest + (nint)2 * (nint)4) = *(int*)(src + (nint)2 * (nint)4);
			*(int*)(dest + (nint)3 * (nint)4) = *(int*)(src + (nint)3 * (nint)4);
			dest += 16;
			src += 16;
			size -= 16;
		}
		while (size >= 4)
		{
			*(int*)dest = *(int*)src;
			dest += 4;
			src += 4;
			size -= 4;
		}
		while (size > 0)
		{
			*dest = *src;
			dest++;
			src++;
			size--;
		}
	}

	internal unsafe static void memcpy2(byte* dest, byte* src, int size)
	{
		while (size >= 8)
		{
			*(short*)dest = *(short*)src;
			*(short*)(dest + 2) = *(short*)(src + 2);
			*(short*)(dest + (nint)2 * (nint)2) = *(short*)(src + (nint)2 * (nint)2);
			*(short*)(dest + (nint)3 * (nint)2) = *(short*)(src + (nint)3 * (nint)2);
			dest += 8;
			src += 8;
			size -= 8;
		}
		while (size >= 2)
		{
			*(short*)dest = *(short*)src;
			dest += 2;
			src += 2;
			size -= 2;
		}
		if (size > 0)
		{
			*dest = *src;
		}
	}

	private unsafe static void memcpy1(byte* dest, byte* src, int size)
	{
		while (size >= 8)
		{
			*dest = *src;
			dest[1] = src[1];
			dest[2] = src[2];
			dest[3] = src[3];
			dest[4] = src[4];
			dest[5] = src[5];
			dest[6] = src[6];
			dest[7] = src[7];
			dest += 8;
			src += 8;
			size -= 8;
		}
		while (size >= 2)
		{
			*dest = *src;
			dest[1] = src[1];
			dest += 2;
			src += 2;
			size -= 2;
		}
		if (size > 0)
		{
			*dest = *src;
		}
	}

	internal unsafe static void Memcpy(byte* dest, byte* src, int size)
	{
		if (((uint)((int)dest | (int)src) & 3u) != 0)
		{
			if (((uint)(int)dest & (true ? 1u : 0u)) != 0 && ((uint)(int)src & (true ? 1u : 0u)) != 0 && size >= 1)
			{
				*dest = *src;
				dest++;
				src++;
				size--;
			}
			if (((uint)(int)dest & 2u) != 0 && ((uint)(int)src & 2u) != 0 && size >= 2)
			{
				*(short*)dest = *(short*)src;
				dest += 2;
				src += 2;
				size -= 2;
			}
			if (((uint)((int)dest | (int)src) & (true ? 1u : 0u)) != 0)
			{
				memcpy1(dest, src, size);
				return;
			}
			if (((uint)((int)dest | (int)src) & 2u) != 0)
			{
				memcpy2(dest, src, size);
				return;
			}
		}
		memcpy4(dest, src, size);
	}
}

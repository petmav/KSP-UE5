using System.Runtime.CompilerServices;

namespace System.Buffers.Binary;

public static class BinaryPrimitives
{
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static sbyte ReverseEndianness(sbyte value)
	{
		return value;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static short ReverseEndianness(short value)
	{
		return (short)(((value & 0xFF) << 8) | ((value & 0xFF00) >> 8));
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static int ReverseEndianness(int value)
	{
		return (int)ReverseEndianness((uint)value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static long ReverseEndianness(long value)
	{
		return (long)ReverseEndianness((ulong)value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static byte ReverseEndianness(byte value)
	{
		return value;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static ushort ReverseEndianness(ushort value)
	{
		return (ushort)((uint)((value & 0xFF) << 8) | ((uint)(value & 0xFF00) >> 8));
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static uint ReverseEndianness(uint value)
	{
		value = (value << 16) | (value >> 16);
		value = ((value & 0xFF00FF) << 8) | ((value & 0xFF00FF00u) >> 8);
		return value;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static ulong ReverseEndianness(ulong value)
	{
		value = (value << 32) | (value >> 32);
		value = ((value & 0xFFFF0000FFFFL) << 16) | ((value & 0xFFFF0000FFFF0000uL) >> 16);
		value = ((value & 0xFF00FF00FF00FFL) << 8) | ((value & 0xFF00FF00FF00FF00uL) >> 8);
		return value;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static T ReadMachineEndian<T>(ReadOnlySpan<byte> buffer) where T : struct
	{
		if (RuntimeHelpers.IsReferenceOrContainsReferences<T>())
		{
			throw new ArgumentException(SR.Format("Cannot use type '{0}'. Only value types without pointers or references are supported.", typeof(T)));
		}
		if (Unsafe.SizeOf<T>() > buffer.Length)
		{
			throw new ArgumentOutOfRangeException();
		}
		return Unsafe.ReadUnaligned<T>(ref buffer.DangerousGetPinnableReference());
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadMachineEndian<T>(ReadOnlySpan<byte> buffer, out T value) where T : struct
	{
		if (RuntimeHelpers.IsReferenceOrContainsReferences<T>())
		{
			throw new ArgumentException(SR.Format("Cannot use type '{0}'. Only value types without pointers or references are supported.", typeof(T)));
		}
		if (Unsafe.SizeOf<T>() > (uint)buffer.Length)
		{
			value = default(T);
			return false;
		}
		value = Unsafe.ReadUnaligned<T>(ref buffer.DangerousGetPinnableReference());
		return true;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static short ReadInt16BigEndian(ReadOnlySpan<byte> buffer)
	{
		short num = ReadMachineEndian<short>(buffer);
		if (BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static int ReadInt32BigEndian(ReadOnlySpan<byte> buffer)
	{
		int num = ReadMachineEndian<int>(buffer);
		if (BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static long ReadInt64BigEndian(ReadOnlySpan<byte> buffer)
	{
		long num = ReadMachineEndian<long>(buffer);
		if (BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static ushort ReadUInt16BigEndian(ReadOnlySpan<byte> buffer)
	{
		ushort num = ReadMachineEndian<ushort>(buffer);
		if (BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static uint ReadUInt32BigEndian(ReadOnlySpan<byte> buffer)
	{
		uint num = ReadMachineEndian<uint>(buffer);
		if (BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static ulong ReadUInt64BigEndian(ReadOnlySpan<byte> buffer)
	{
		ulong num = ReadMachineEndian<ulong>(buffer);
		if (BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadInt16BigEndian(ReadOnlySpan<byte> buffer, out short value)
	{
		bool result = TryReadMachineEndian<short>(buffer, out value);
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadInt32BigEndian(ReadOnlySpan<byte> buffer, out int value)
	{
		bool result = TryReadMachineEndian<int>(buffer, out value);
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadInt64BigEndian(ReadOnlySpan<byte> buffer, out long value)
	{
		bool result = TryReadMachineEndian<long>(buffer, out value);
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadUInt16BigEndian(ReadOnlySpan<byte> buffer, out ushort value)
	{
		bool result = TryReadMachineEndian<ushort>(buffer, out value);
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadUInt32BigEndian(ReadOnlySpan<byte> buffer, out uint value)
	{
		bool result = TryReadMachineEndian<uint>(buffer, out value);
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadUInt64BigEndian(ReadOnlySpan<byte> buffer, out ulong value)
	{
		bool result = TryReadMachineEndian<ulong>(buffer, out value);
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static short ReadInt16LittleEndian(ReadOnlySpan<byte> buffer)
	{
		short num = ReadMachineEndian<short>(buffer);
		if (!BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static int ReadInt32LittleEndian(ReadOnlySpan<byte> buffer)
	{
		int num = ReadMachineEndian<int>(buffer);
		if (!BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static long ReadInt64LittleEndian(ReadOnlySpan<byte> buffer)
	{
		long num = ReadMachineEndian<long>(buffer);
		if (!BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static ushort ReadUInt16LittleEndian(ReadOnlySpan<byte> buffer)
	{
		ushort num = ReadMachineEndian<ushort>(buffer);
		if (!BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static uint ReadUInt32LittleEndian(ReadOnlySpan<byte> buffer)
	{
		uint num = ReadMachineEndian<uint>(buffer);
		if (!BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static ulong ReadUInt64LittleEndian(ReadOnlySpan<byte> buffer)
	{
		ulong num = ReadMachineEndian<ulong>(buffer);
		if (!BitConverter.IsLittleEndian)
		{
			num = ReverseEndianness(num);
		}
		return num;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadInt16LittleEndian(ReadOnlySpan<byte> buffer, out short value)
	{
		bool result = TryReadMachineEndian<short>(buffer, out value);
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadInt32LittleEndian(ReadOnlySpan<byte> buffer, out int value)
	{
		bool result = TryReadMachineEndian<int>(buffer, out value);
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadInt64LittleEndian(ReadOnlySpan<byte> buffer, out long value)
	{
		bool result = TryReadMachineEndian<long>(buffer, out value);
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadUInt16LittleEndian(ReadOnlySpan<byte> buffer, out ushort value)
	{
		bool result = TryReadMachineEndian<ushort>(buffer, out value);
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadUInt32LittleEndian(ReadOnlySpan<byte> buffer, out uint value)
	{
		bool result = TryReadMachineEndian<uint>(buffer, out value);
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryReadUInt64LittleEndian(ReadOnlySpan<byte> buffer, out ulong value)
	{
		bool result = TryReadMachineEndian<ulong>(buffer, out value);
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return result;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteMachineEndian<T>(Span<byte> buffer, ref T value) where T : struct
	{
		if (RuntimeHelpers.IsReferenceOrContainsReferences<T>())
		{
			throw new ArgumentException(SR.Format("Cannot use type '{0}'. Only value types without pointers or references are supported.", typeof(T)));
		}
		if ((uint)Unsafe.SizeOf<T>() > (uint)buffer.Length)
		{
			throw new ArgumentOutOfRangeException();
		}
		Unsafe.WriteUnaligned(ref buffer.DangerousGetPinnableReference(), value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteMachineEndian<T>(Span<byte> buffer, ref T value) where T : struct
	{
		if (RuntimeHelpers.IsReferenceOrContainsReferences<T>())
		{
			throw new ArgumentException(SR.Format("Cannot use type '{0}'. Only value types without pointers or references are supported.", typeof(T)));
		}
		if (Unsafe.SizeOf<T>() > (uint)buffer.Length)
		{
			return false;
		}
		Unsafe.WriteUnaligned(ref buffer.DangerousGetPinnableReference(), value);
		return true;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteInt16BigEndian(Span<byte> buffer, short value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteInt32BigEndian(Span<byte> buffer, int value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteInt64BigEndian(Span<byte> buffer, long value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteUInt16BigEndian(Span<byte> buffer, ushort value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteUInt32BigEndian(Span<byte> buffer, uint value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteUInt64BigEndian(Span<byte> buffer, ulong value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteInt16BigEndian(Span<byte> buffer, short value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteInt32BigEndian(Span<byte> buffer, int value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteInt64BigEndian(Span<byte> buffer, long value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteUInt16BigEndian(Span<byte> buffer, ushort value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteUInt32BigEndian(Span<byte> buffer, uint value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteUInt64BigEndian(Span<byte> buffer, ulong value)
	{
		if (BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteInt16LittleEndian(Span<byte> buffer, short value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteInt32LittleEndian(Span<byte> buffer, int value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteInt64LittleEndian(Span<byte> buffer, long value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteUInt16LittleEndian(Span<byte> buffer, ushort value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteUInt32LittleEndian(Span<byte> buffer, uint value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static void WriteUInt64LittleEndian(Span<byte> buffer, ulong value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		WriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteInt16LittleEndian(Span<byte> buffer, short value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteInt32LittleEndian(Span<byte> buffer, int value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteInt64LittleEndian(Span<byte> buffer, long value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteUInt16LittleEndian(Span<byte> buffer, ushort value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteUInt32LittleEndian(Span<byte> buffer, uint value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static bool TryWriteUInt64LittleEndian(Span<byte> buffer, ulong value)
	{
		if (!BitConverter.IsLittleEndian)
		{
			value = ReverseEndianness(value);
		}
		return TryWriteMachineEndian(buffer, ref value);
	}
}

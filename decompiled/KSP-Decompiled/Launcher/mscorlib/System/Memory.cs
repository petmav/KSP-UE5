using System.Buffers;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace System;

[DebuggerDisplay("{DebuggerDisplay,nq}")]
[DebuggerTypeProxy(typeof(MemoryDebugView<>))]
public readonly struct Memory<T>
{
	private readonly object _arrayOrOwnedMemory;

	private readonly int _index;

	private readonly int _length;

	private const int RemoveOwnedFlagBitMask = int.MaxValue;

	private string DebuggerDisplay => $"{{{typeof(T).Name}[{_length}]}}";

	public static Memory<T> Empty { get; } = SpanHelpers.PerTypeValues<T>.EmptyArray;


	public int Length => _length;

	public bool IsEmpty => _length == 0;

	public Span<T> Span
	{
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get
		{
			if (_index < 0)
			{
				return ((OwnedMemory<T>)_arrayOrOwnedMemory).Span.Slice(_index & 0x7FFFFFFF, _length);
			}
			return new Span<T>((T[])_arrayOrOwnedMemory, _index, _length);
		}
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public Memory(T[] array)
	{
		if (array == null)
		{
			ThrowHelper.ThrowArgumentNullException(ExceptionArgument.array);
		}
		if (default(T) == null && array.GetType() != typeof(T[]))
		{
			ThrowHelper.ThrowArrayTypeMismatchException_ArrayTypeMustBeExactMatch(typeof(T));
		}
		_arrayOrOwnedMemory = array;
		_index = 0;
		_length = array.Length;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public Memory(T[] array, int start, int length)
	{
		if (array == null)
		{
			ThrowHelper.ThrowArgumentNullException(ExceptionArgument.array);
		}
		if (default(T) == null && array.GetType() != typeof(T[]))
		{
			ThrowHelper.ThrowArrayTypeMismatchException_ArrayTypeMustBeExactMatch(typeof(T));
		}
		if ((uint)start > (uint)array.Length || (uint)length > (uint)(array.Length - start))
		{
			ThrowHelper.ThrowArgumentOutOfRangeException(ExceptionArgument.start);
		}
		_arrayOrOwnedMemory = array;
		_index = start;
		_length = length;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	internal Memory(OwnedMemory<T> owner, int index, int length)
	{
		if (owner == null)
		{
			ThrowHelper.ThrowArgumentNullException(ExceptionArgument.ownedMemory);
		}
		if (index < 0 || length < 0)
		{
			ThrowHelper.ThrowArgumentOutOfRangeException(ExceptionArgument.start);
		}
		_arrayOrOwnedMemory = owner;
		_index = index | int.MinValue;
		_length = length;
	}

	public static implicit operator Memory<T>(T[] array)
	{
		return new Memory<T>(array);
	}

	public static implicit operator Memory<T>(ArraySegment<T> arraySegment)
	{
		return new Memory<T>(arraySegment.Array, arraySegment.Offset, arraySegment.Count);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static implicit operator ReadOnlyMemory<T>(Memory<T> memory)
	{
		if (memory._index < 0)
		{
			return new ReadOnlyMemory<T>((OwnedMemory<T>)memory._arrayOrOwnedMemory, memory._index & 0x7FFFFFFF, memory._length);
		}
		return new ReadOnlyMemory<T>((T[])memory._arrayOrOwnedMemory, memory._index, memory._length);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public Memory<T> Slice(int start)
	{
		if ((uint)start > (uint)_length)
		{
			ThrowHelper.ThrowArgumentOutOfRangeException(ExceptionArgument.start);
		}
		if (_index < 0)
		{
			return new Memory<T>((OwnedMemory<T>)_arrayOrOwnedMemory, (_index & 0x7FFFFFFF) + start, _length - start);
		}
		return new Memory<T>((T[])_arrayOrOwnedMemory, _index + start, _length - start);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public Memory<T> Slice(int start, int length)
	{
		if ((uint)start > (uint)_length || (uint)length > (uint)(_length - start))
		{
			ThrowHelper.ThrowArgumentOutOfRangeException(ExceptionArgument.start);
		}
		if (_index < 0)
		{
			return new Memory<T>((OwnedMemory<T>)_arrayOrOwnedMemory, (_index & 0x7FFFFFFF) + start, length);
		}
		return new Memory<T>((T[])_arrayOrOwnedMemory, _index + start, length);
	}

	public unsafe MemoryHandle Retain(bool pin = false)
	{
		MemoryHandle result;
		if (pin)
		{
			if (_index < 0)
			{
				result = ((OwnedMemory<T>)_arrayOrOwnedMemory).Pin();
				result.AddOffset((_index & 0x7FFFFFFF) * Unsafe.SizeOf<T>());
			}
			else
			{
				GCHandle handle = GCHandle.Alloc((T[])_arrayOrOwnedMemory, GCHandleType.Pinned);
				void* pinnedPointer = Unsafe.Add<T>((void*)handle.AddrOfPinnedObject(), _index);
				result = new MemoryHandle(null, pinnedPointer, handle);
			}
		}
		else if (_index < 0)
		{
			((OwnedMemory<T>)_arrayOrOwnedMemory).Retain();
			result = new MemoryHandle((OwnedMemory<T>)_arrayOrOwnedMemory, null);
		}
		else
		{
			result = new MemoryHandle(null, null);
		}
		return result;
	}

	public bool TryGetArray(out ArraySegment<T> arraySegment)
	{
		if (_index < 0)
		{
			if (((OwnedMemory<T>)_arrayOrOwnedMemory).TryGetArray(out var arraySegment2))
			{
				arraySegment = new ArraySegment<T>(arraySegment2.Array, arraySegment2.Offset + (_index & 0x7FFFFFFF), _length);
				return true;
			}
			arraySegment = default(ArraySegment<T>);
			return false;
		}
		arraySegment = new ArraySegment<T>((T[])_arrayOrOwnedMemory, _index, _length);
		return true;
	}

	public T[] ToArray()
	{
		return Span.ToArray();
	}

	public override bool Equals(object obj)
	{
		if (obj is ReadOnlyMemory<T> readOnlyMemory)
		{
			return readOnlyMemory.Equals(this);
		}
		bool num = obj is Memory<T>;
		Memory<T> other = (num ? ((Memory<T>)obj) : default(Memory<T>));
		if (num)
		{
			return Equals(other);
		}
		return false;
	}

	public bool Equals(Memory<T> other)
	{
		if (_arrayOrOwnedMemory == other._arrayOrOwnedMemory && _index == other._index)
		{
			return _length == other._length;
		}
		return false;
	}

	public override int GetHashCode()
	{
		return CombineHashCodes(_arrayOrOwnedMemory.GetHashCode(), (_index & 0x7FFFFFFF).GetHashCode(), _length.GetHashCode());
	}

	private static int CombineHashCodes(int left, int right)
	{
		return ((left << 5) + left) ^ right;
	}

	private static int CombineHashCodes(int h1, int h2, int h3)
	{
		return CombineHashCodes(CombineHashCodes(h1, h2), h3);
	}
}

using System.Diagnostics;
using System.Runtime.CompilerServices;

namespace System;

[DebuggerTypeProxy(typeof(SpanDebugView<>))]
[DebuggerDisplay("{DebuggerDisplay,nq}")]
public readonly ref struct Span<T>
{
	private readonly Pinnable<T> _pinnable;

	private readonly IntPtr _byteOffset;

	private readonly int _length;

	private string DebuggerDisplay => $"{{{typeof(T).Name}[{_length}]}}";

	public int Length => _length;

	public bool IsEmpty => _length == 0;

	public unsafe ref T this[int index]
	{
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get
		{
			if ((uint)index >= (uint)_length)
			{
				ThrowHelper.ThrowIndexOutOfRangeException();
			}
			if (_pinnable == null)
			{
				return ref Unsafe.Add(ref Unsafe.AsRef<T>(_byteOffset.ToPointer()), index);
			}
			return ref Unsafe.Add(ref Unsafe.AddByteOffset(ref _pinnable.Data, _byteOffset), index);
		}
	}

	public static Span<T> Empty => default(Span<T>);

	internal Pinnable<T> Pinnable => _pinnable;

	internal IntPtr ByteOffset => _byteOffset;

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public Span(T[] array)
	{
		if (array == null)
		{
			ThrowHelper.ThrowArgumentNullException(ExceptionArgument.array);
		}
		if (default(T) == null && array.GetType() != typeof(T[]))
		{
			ThrowHelper.ThrowArrayTypeMismatchException_ArrayTypeMustBeExactMatch(typeof(T));
		}
		_length = array.Length;
		_pinnable = Unsafe.As<Pinnable<T>>(array);
		_byteOffset = SpanHelpers.PerTypeValues<T>.ArrayAdjustment;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public Span(T[] array, int start, int length)
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
		_length = length;
		_pinnable = Unsafe.As<Pinnable<T>>(array);
		_byteOffset = SpanHelpers.PerTypeValues<T>.ArrayAdjustment.Add<T>(start);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public unsafe Span(void* pointer, int length)
	{
		if (SpanHelpers.IsReferenceOrContainsReferences<T>())
		{
			ThrowHelper.ThrowArgumentException_InvalidTypeWithPointersNotSupported(typeof(T));
		}
		if (length < 0)
		{
			ThrowHelper.ThrowArgumentOutOfRangeException(ExceptionArgument.start);
		}
		_length = length;
		_pinnable = null;
		_byteOffset = new IntPtr(pointer);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static Span<T> DangerousCreate(object obj, ref T objectData, int length)
	{
		Pinnable<T> pinnable = Unsafe.As<Pinnable<T>>(obj);
		IntPtr byteOffset = Unsafe.ByteOffset(ref pinnable.Data, ref objectData);
		return new Span<T>(pinnable, byteOffset, length);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	internal Span(Pinnable<T> pinnable, IntPtr byteOffset, int length)
	{
		_length = length;
		_pinnable = pinnable;
		_byteOffset = byteOffset;
	}

	public unsafe void Clear()
	{
		int length = _length;
		if (length == 0)
		{
			return;
		}
		UIntPtr byteLength = (UIntPtr)(ulong)((uint)length * Unsafe.SizeOf<T>());
		if ((Unsafe.SizeOf<T>() & (sizeof(IntPtr) - 1)) != 0)
		{
			if (_pinnable == null)
			{
				byte* ptr = (byte*)_byteOffset.ToPointer();
				SpanHelpers.ClearLessThanPointerSized(ptr, byteLength);
			}
			else
			{
				SpanHelpers.ClearLessThanPointerSized(ref Unsafe.As<T, byte>(ref Unsafe.AddByteOffset(ref _pinnable.Data, _byteOffset)), byteLength);
			}
		}
		else if (SpanHelpers.IsReferenceOrContainsReferences<T>())
		{
			UIntPtr pointerSizeLength = (UIntPtr)(ulong)(length * Unsafe.SizeOf<T>() / sizeof(IntPtr));
			SpanHelpers.ClearPointerSizedWithReferences(ref Unsafe.As<T, IntPtr>(ref DangerousGetPinnableReference()), pointerSizeLength);
		}
		else
		{
			SpanHelpers.ClearPointerSizedWithoutReferences(ref Unsafe.As<T, byte>(ref DangerousGetPinnableReference()), byteLength);
		}
	}

	public unsafe void Fill(T value)
	{
		int length = _length;
		if (length == 0)
		{
			return;
		}
		if (Unsafe.SizeOf<T>() == 1)
		{
			byte value2 = Unsafe.As<T, byte>(ref value);
			if (_pinnable == null)
			{
				Unsafe.InitBlockUnaligned(_byteOffset.ToPointer(), value2, (uint)length);
			}
			else
			{
				Unsafe.InitBlockUnaligned(ref Unsafe.As<T, byte>(ref Unsafe.AddByteOffset(ref _pinnable.Data, _byteOffset)), value2, (uint)length);
			}
			return;
		}
		ref T source = ref DangerousGetPinnableReference();
		int i;
		for (i = 0; i < (length & -8); i += 8)
		{
			Unsafe.Add(ref source, i) = value;
			Unsafe.Add(ref source, i + 1) = value;
			Unsafe.Add(ref source, i + 2) = value;
			Unsafe.Add(ref source, i + 3) = value;
			Unsafe.Add(ref source, i + 4) = value;
			Unsafe.Add(ref source, i + 5) = value;
			Unsafe.Add(ref source, i + 6) = value;
			Unsafe.Add(ref source, i + 7) = value;
		}
		if (i < (length & -4))
		{
			Unsafe.Add(ref source, i) = value;
			Unsafe.Add(ref source, i + 1) = value;
			Unsafe.Add(ref source, i + 2) = value;
			Unsafe.Add(ref source, i + 3) = value;
			i += 4;
		}
		for (; i < length; i++)
		{
			Unsafe.Add(ref source, i) = value;
		}
	}

	public void CopyTo(Span<T> destination)
	{
		if (!TryCopyTo(destination))
		{
			ThrowHelper.ThrowArgumentException_DestinationTooShort();
		}
	}

	public bool TryCopyTo(Span<T> destination)
	{
		int length = _length;
		int length2 = destination._length;
		if (length == 0)
		{
			return true;
		}
		if ((uint)length > (uint)length2)
		{
			return false;
		}
		ref T src = ref DangerousGetPinnableReference();
		SpanHelpers.CopyTo(ref destination.DangerousGetPinnableReference(), length2, ref src, length);
		return true;
	}

	public static bool operator ==(Span<T> left, Span<T> right)
	{
		if (left._length == right._length)
		{
			return Unsafe.AreSame(ref left.DangerousGetPinnableReference(), ref right.DangerousGetPinnableReference());
		}
		return false;
	}

	public static bool operator !=(Span<T> left, Span<T> right)
	{
		return !(left == right);
	}

	[Obsolete("Equals() on Span will always throw an exception. Use == instead.")]
	public override bool Equals(object obj)
	{
		throw new NotSupportedException("Equals() on Span and ReadOnlySpan is not supported. Use operator== instead.");
	}

	[Obsolete("GetHashCode() on Span will always throw an exception.")]
	public override int GetHashCode()
	{
		throw new NotSupportedException("GetHashCode() on Span and ReadOnlySpan is not supported.");
	}

	public static implicit operator Span<T>(T[] array)
	{
		return new Span<T>(array);
	}

	public static implicit operator Span<T>(ArraySegment<T> arraySegment)
	{
		return new Span<T>(arraySegment.Array, arraySegment.Offset, arraySegment.Count);
	}

	public static implicit operator ReadOnlySpan<T>(Span<T> span)
	{
		return new ReadOnlySpan<T>(span._pinnable, span._byteOffset, span._length);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public Span<T> Slice(int start)
	{
		if ((uint)start > (uint)_length)
		{
			ThrowHelper.ThrowArgumentOutOfRangeException(ExceptionArgument.start);
		}
		IntPtr byteOffset = _byteOffset.Add<T>(start);
		int length = _length - start;
		return new Span<T>(_pinnable, byteOffset, length);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public Span<T> Slice(int start, int length)
	{
		if ((uint)start > (uint)_length || (uint)length > (uint)(_length - start))
		{
			ThrowHelper.ThrowArgumentOutOfRangeException(ExceptionArgument.start);
		}
		IntPtr byteOffset = _byteOffset.Add<T>(start);
		return new Span<T>(_pinnable, byteOffset, length);
	}

	public T[] ToArray()
	{
		if (_length == 0)
		{
			return SpanHelpers.PerTypeValues<T>.EmptyArray;
		}
		T[] array = new T[_length];
		CopyTo(array);
		return array;
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public unsafe ref T DangerousGetPinnableReference()
	{
		if (_pinnable == null)
		{
			return ref Unsafe.AsRef<T>(_byteOffset.ToPointer());
		}
		return ref Unsafe.AddByteOffset(ref _pinnable.Data, _byteOffset);
	}
}

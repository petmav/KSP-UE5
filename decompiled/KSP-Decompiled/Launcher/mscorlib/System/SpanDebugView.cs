using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace System;

internal sealed class SpanDebugView<T>
{
	private readonly T[] _pinnable;

	private readonly IntPtr _byteOffset;

	private readonly int _length;

	[DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
	public unsafe T[] Items
	{
		get
		{
			int num = (typeof(T).GetTypeInfo().IsValueType ? Unsafe.SizeOf<T>() : IntPtr.Size);
			T[] array = new T[_length];
			if (_pinnable == null)
			{
				byte* ptr = (byte*)_byteOffset.ToPointer();
				for (int i = 0; i < array.Length; i++)
				{
					array[i] = Unsafe.Read<T>(ptr);
					ptr += num;
				}
			}
			else
			{
				long num2 = _byteOffset.ToInt64();
				long num3 = SpanHelpers.PerTypeValues<T>.ArrayAdjustment.ToInt64();
				int sourceIndex = (int)((num2 - num3) / num);
				Array.Copy(_pinnable, sourceIndex, array, 0, _length);
			}
			return array;
		}
	}

	public SpanDebugView(Span<T> collection)
	{
		_pinnable = (T[])(object)collection.Pinnable;
		_byteOffset = collection.ByteOffset;
		_length = collection.Length;
	}

	public SpanDebugView(ReadOnlySpan<T> collection)
	{
		_pinnable = (T[])(object)collection.Pinnable;
		_byteOffset = collection.ByteOffset;
		_length = collection.Length;
	}
}

using System.Diagnostics;

namespace System;

internal sealed class MemoryDebugView<T>
{
	private readonly ReadOnlyMemory<T> _memory;

	[DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
	public T[] Items
	{
		get
		{
			ReadOnlyMemory<T> memory = _memory;
			if (memory.DangerousTryGetArray(out var arraySegment))
			{
				memory = _memory;
				T[] array = new T[memory.Length];
				Array.Copy(arraySegment.Array, arraySegment.Offset, array, 0, array.Length);
				return array;
			}
			return SpanHelpers.PerTypeValues<T>.EmptyArray;
		}
	}

	public MemoryDebugView(Memory<T> memory)
	{
		_memory = memory;
	}

	public MemoryDebugView(ReadOnlyMemory<T> memory)
	{
		_memory = memory;
	}
}

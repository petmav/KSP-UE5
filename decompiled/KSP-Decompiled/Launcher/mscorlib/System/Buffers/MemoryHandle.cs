using System.Runtime.InteropServices;

namespace System.Buffers;

public struct MemoryHandle : IDisposable
{
	private IRetainable _retainable;

	private unsafe void* _pointer;

	private GCHandle _handle;

	public unsafe void* PinnedPointer => _pointer;

	public unsafe MemoryHandle(IRetainable retainable, void* pinnedPointer = null, GCHandle handle = default(GCHandle))
	{
		_retainable = retainable;
		_pointer = pinnedPointer;
		_handle = handle;
	}

	internal unsafe void AddOffset(int offset)
	{
		if (_pointer == null)
		{
			ThrowHelper.ThrowArgumentNullException(ExceptionArgument.pointer);
		}
		else
		{
			_pointer = (byte*)_pointer + offset;
		}
	}

	public unsafe void Dispose()
	{
		if (_handle.IsAllocated)
		{
			_handle.Free();
		}
		if (_retainable != null)
		{
			_retainable.Release();
			_retainable = null;
		}
		_pointer = null;
	}
}

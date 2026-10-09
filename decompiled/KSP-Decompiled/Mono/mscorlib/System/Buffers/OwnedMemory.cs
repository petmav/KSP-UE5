namespace System.Buffers;

public abstract class OwnedMemory<T> : IDisposable, IRetainable
{
	public abstract int Length { get; }

	public abstract Span<T> Span { get; }

	public Memory<T> Memory
	{
		get
		{
			if (IsDisposed)
			{
				ThrowHelper.ThrowObjectDisposedException_MemoryDisposed("OwnedMemory");
			}
			return new Memory<T>(this, 0, Length);
		}
	}

	protected abstract bool IsRetained { get; }

	public abstract bool IsDisposed { get; }

	public abstract MemoryHandle Pin();

	protected internal abstract bool TryGetArray(out ArraySegment<T> arraySegment);

	public void Dispose()
	{
		if (IsRetained)
		{
			ThrowHelper.ThrowInvalidOperationException_OutstandingReferences();
		}
		Dispose(disposing: true);
		GC.SuppressFinalize(this);
	}

	protected abstract void Dispose(bool disposing);

	public abstract void Retain();

	public abstract bool Release();
}

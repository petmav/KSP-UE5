using System.Runtime.CompilerServices;
using System.Threading;

namespace System.Buffers;

internal abstract class ArrayPool<T>
{
	private static System.Buffers.ArrayPool<T> s_sharedInstance;

	public static System.Buffers.ArrayPool<T> Shared
	{
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get
		{
			return Volatile.Read(ref s_sharedInstance) ?? EnsureSharedCreated();
		}
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	private static System.Buffers.ArrayPool<T> EnsureSharedCreated()
	{
		Interlocked.CompareExchange(ref s_sharedInstance, Create(), null);
		return s_sharedInstance;
	}

	public static System.Buffers.ArrayPool<T> Create()
	{
		return new System.Buffers.DefaultArrayPool<T>();
	}

	public static System.Buffers.ArrayPool<T> Create(int maxArrayLength, int maxArraysPerBucket)
	{
		return new System.Buffers.DefaultArrayPool<T>(maxArrayLength, maxArraysPerBucket);
	}

	public abstract T[] Rent(int minimumLength);

	public abstract void Return(T[] array, bool clearArray = false);
}

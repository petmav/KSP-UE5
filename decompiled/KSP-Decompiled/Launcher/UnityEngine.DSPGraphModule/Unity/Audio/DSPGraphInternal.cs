using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Unity.Jobs;
using UnityEngine.Bindings;

namespace Unity.Audio;

[StructLayout(LayoutKind.Sequential, Size = 1)]
[NativeType(Header = "Modules/DSPGraph/Public/DSPGraph.bindings.h")]
internal struct DSPGraphInternal
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern void Internal_CreateDSPGraph(out Handle graph, int outputFormat, uint outputChannels, uint dspBufferSize, uint sampleRate);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern void Internal_DisposeDSPGraph(ref Handle graph);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern void Internal_CreateDSPCommandBlock(ref Handle graph, ref Handle block);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern uint Internal_AddNodeEventHandler(ref Handle graph, long eventTypeHashCode, object handler);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern bool Internal_RemoveNodeEventHandler(ref Handle graph, uint handlerId);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern void Internal_GetRootDSP(ref Handle graph, ref Handle root);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern ulong Internal_GetDSPClock(ref Handle graph);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public static extern void Internal_BeginMix(ref Handle graph, int frameCount, int executionMode);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public unsafe static extern void Internal_ReadMix(ref Handle graph, void* buffer, int frameCount);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern void Internal_Update(ref Handle graph);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public static extern bool Internal_AssertMixerThread(ref Handle graph);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public static extern bool Internal_AssertMainThread(ref Handle graph);

	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public static Handle Internal_AllocateHandle(ref Handle graph)
	{
		Internal_AllocateHandle_Injected(ref graph, out var ret);
		return ret;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public unsafe static extern void Internal_InitializeJob(void* jobStructData, void* jobReflectionData, void* resourceContext);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public unsafe static extern void Internal_ExecuteJob(void* jobStructData, void* jobReflectionData, void* jobData, void* resourceContext);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public unsafe static extern void Internal_ExecuteUpdateJob(void* updateStructMemory, void* updateReflectionData, void* jobStructMemory, void* jobReflectionData, void* resourceContext, ref Handle requestHandle, ref JobHandle fence);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public unsafe static extern void Internal_DisposeJob(void* jobStructData, void* jobReflectionData, void* resourceContext);

	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public unsafe static void Internal_ScheduleGraph(JobHandle inputDeps, void* nodes, int nodeCount, int* childTable, void* dependencies)
	{
		Internal_ScheduleGraph_Injected(ref inputDeps, nodes, nodeCount, childTable, dependencies);
	}

	[NativeMethod(IsFreeFunction = true, ThrowsException = true, IsThreadSafe = true)]
	public static void Internal_SyncFenceNoWorkSteal(JobHandle handle)
	{
		Internal_SyncFenceNoWorkSteal_Injected(ref handle);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void Internal_AllocateHandle_Injected(ref Handle graph, out Handle ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private unsafe static extern void Internal_ScheduleGraph_Injected(ref JobHandle inputDeps, void* nodes, int nodeCount, int* childTable, void* dependencies);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void Internal_SyncFenceNoWorkSteal_Injected(ref JobHandle handle);
}

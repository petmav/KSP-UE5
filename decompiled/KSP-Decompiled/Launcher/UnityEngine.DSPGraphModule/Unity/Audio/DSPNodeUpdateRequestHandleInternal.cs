using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Unity.Jobs;
using UnityEngine.Bindings;

namespace Unity.Audio;

[StructLayout(LayoutKind.Sequential, Size = 1)]
[NativeType(Header = "Modules/DSPGraph/Public/DSPNodeUpdateRequest.bindings.h")]
internal struct DSPNodeUpdateRequestHandleInternal
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public unsafe static extern void* Internal_GetUpdateJobData(ref Handle graph, ref Handle requestHandle);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern bool Internal_HasError(ref Handle graph, ref Handle requestHandle);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern void Internal_GetDSPNode(ref Handle graph, ref Handle requestHandle, ref Handle node);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern void Internal_GetFence(ref Handle graph, ref Handle requestHandle, ref JobHandle fence);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern void Internal_Dispose(ref Handle graph, ref Handle requestHandle);
}

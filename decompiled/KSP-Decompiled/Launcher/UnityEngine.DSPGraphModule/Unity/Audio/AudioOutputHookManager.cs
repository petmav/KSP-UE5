using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;

namespace Unity.Audio;

[StructLayout(LayoutKind.Sequential, Size = 1)]
[NativeType(Header = "Modules/DSPGraph/Public/AudioOutputHookManager.bindings.h")]
internal struct AudioOutputHookManager
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public unsafe static extern void Internal_CreateAudioOutputHook(out Handle outputHook, void* jobReflectionData, void* jobData);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern void Internal_DisposeAudioOutputHook(ref Handle outputHook);
}

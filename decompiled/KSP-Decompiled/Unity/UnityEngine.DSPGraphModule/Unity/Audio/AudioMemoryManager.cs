using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;

namespace Unity.Audio;

[StructLayout(LayoutKind.Sequential, Size = 1)]
[NativeType(Header = "Modules/DSPGraph/Public/AudioMemoryManager.bindings.h")]
internal struct AudioMemoryManager
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = false)]
	public unsafe static extern void* Internal_AllocateAudioMemory(int size, int alignment);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = false)]
	public unsafe static extern void Internal_FreeAudioMemory(void* memory);
}

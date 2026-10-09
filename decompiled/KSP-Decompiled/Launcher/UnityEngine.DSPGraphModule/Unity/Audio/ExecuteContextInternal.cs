using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;

namespace Unity.Audio;

[StructLayout(LayoutKind.Sequential, Size = 1)]
[NativeType(Header = "Modules/DSPGraph/Public/ExecuteContext.bindings.h")]
internal struct ExecuteContextInternal
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true)]
	public unsafe static extern void Internal_PostEvent(void* dspNodePtr, long eventTypeHashCode, void* eventPtr, int eventSize);
}

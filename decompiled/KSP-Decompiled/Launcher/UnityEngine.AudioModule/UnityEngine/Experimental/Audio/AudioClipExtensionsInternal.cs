using System.Runtime.CompilerServices;
using UnityEngine.Bindings;

namespace UnityEngine.Experimental.Audio;

[NativeHeader("Modules/Audio/Public/ScriptBindings/AudioClipExtensions.bindings.h")]
[NativeHeader("Modules/Audio/Public/AudioClip.h")]
[NativeHeader("AudioScriptingClasses.h")]
internal static class AudioClipExtensionsInternal
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsFreeFunction = true, ThrowsException = true)]
	public static extern uint Internal_CreateAudioClipSampleProvider(this AudioClip audioClip, ulong start, long end, bool loop, bool allowDrop);
}

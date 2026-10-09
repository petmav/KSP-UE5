using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;

namespace Unity.Audio;

[StructLayout(LayoutKind.Sequential, Size = 1)]
[NativeType(Header = "Modules/DSPGraph/Public/DSPSampleProvider.bindings.h")]
internal struct DSPSampleProviderInternal
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true, ThrowsException = true)]
	public unsafe static extern int Internal_ReadUInt8FromSampleProvider(void* provider, int format, void* buffer, int length);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true, ThrowsException = true)]
	public unsafe static extern int Internal_ReadSInt16FromSampleProvider(void* provider, int format, void* buffer, int length);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true, ThrowsException = true)]
	public unsafe static extern int Internal_ReadFloatFromSampleProvider(void* provider, void* buffer, int length);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true, ThrowsException = true)]
	public unsafe static extern ushort Internal_GetChannelCount(void* provider);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true, ThrowsException = true)]
	public unsafe static extern uint Internal_GetSampleRate(void* provider);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true, ThrowsException = true)]
	public unsafe static extern int Internal_ReadUInt8FromSampleProviderById(uint providerId, int format, void* buffer, int length);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true, ThrowsException = true)]
	public unsafe static extern int Internal_ReadSInt16FromSampleProviderById(uint providerId, int format, void* buffer, int length);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true, ThrowsException = true)]
	public unsafe static extern int Internal_ReadFloatFromSampleProviderById(uint providerId, void* buffer, int length);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true, ThrowsException = true)]
	public static extern ushort Internal_GetChannelCountById(uint providerId);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true, IsFreeFunction = true, ThrowsException = true)]
	public static extern uint Internal_GetSampleRateById(uint providerId);
}

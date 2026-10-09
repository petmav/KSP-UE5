using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;
using UnityEngine.Scripting;

namespace UnityEngine.Profiling;

[StructLayout(LayoutKind.Sequential)]
[NativeHeader("Runtime/Profiler/Recorder.h")]
[UsedByNativeCode]
[NativeHeader("Runtime/Profiler/ScriptBindings/Recorder.bindings.h")]
public sealed class Recorder
{
	internal IntPtr m_Ptr;

	internal static Recorder s_InvalidRecorder = new Recorder();

	public bool isValid => m_Ptr != IntPtr.Zero;

	public bool enabled
	{
		get
		{
			return isValid && IsEnabled();
		}
		set
		{
			if (isValid)
			{
				SetEnabled(value);
			}
		}
	}

	public long elapsedNanoseconds => isValid ? GetElapsedNanoseconds() : 0;

	public int sampleBlockCount => isValid ? GetSampleBlockCount() : 0;

	internal Recorder()
	{
	}

	internal Recorder(IntPtr ptr)
	{
		m_Ptr = ptr;
	}

	~Recorder()
	{
		if (m_Ptr != IntPtr.Zero)
		{
			DisposeNative(m_Ptr);
		}
	}

	public static Recorder Get(string samplerName)
	{
		IntPtr @internal = GetInternal(samplerName);
		if (@internal == IntPtr.Zero)
		{
			return s_InvalidRecorder;
		}
		return new Recorder(@internal);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(Name = "ProfilerBindings::GetRecorderInternal", IsFreeFunction = true)]
	private static extern IntPtr GetInternal(string samplerName);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(Name = "ProfilerBindings::DisposeNativeRecorder", IsFreeFunction = true, IsThreadSafe = true)]
	private static extern void DisposeNative(IntPtr ptr);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true)]
	private extern bool IsEnabled();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true)]
	private extern void SetEnabled(bool enabled);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true)]
	private extern long GetElapsedNanoseconds();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod(IsThreadSafe = true)]
	private extern int GetSampleBlockCount();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[ThreadSafe]
	public extern void FilterToCurrentThread();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[ThreadSafe]
	public extern void CollectFromAllThreads();
}

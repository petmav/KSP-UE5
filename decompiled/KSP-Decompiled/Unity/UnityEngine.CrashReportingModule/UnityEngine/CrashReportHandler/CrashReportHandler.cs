using System.Runtime.CompilerServices;
using UnityEngine.Bindings;

namespace UnityEngine.CrashReportHandler;

[StaticAccessor("CrashReporting::CrashReporter::Get()", StaticAccessorType.Dot)]
[NativeHeader("Modules/CrashReporting/Public/CrashReporter.h")]
public class CrashReportHandler
{
	[NativeProperty("Enabled")]
	public static extern bool enableCaptureExceptions
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	[NativeThrows]
	public static extern uint logBufferSize
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
		[MethodImpl(MethodImplOptions.InternalCall)]
		set;
	}

	private CrashReportHandler()
	{
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeThrows]
	public static extern string GetUserMetadata(string key);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeThrows]
	public static extern void SetUserMetadata(string key, string value);
}

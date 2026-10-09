using System.Runtime.CompilerServices;
using UnityEngine.Bindings;
using UnityEngine.Scripting.APIUpdating;

namespace UnityEngine.Windows.WebCam;

[StaticAccessor("WebCam::GetInstance()", StaticAccessorType.Dot)]
[MovedFrom("UnityEngine.XR.WSA.WebCam")]
[NativeHeader("PlatformDependent/Win/Webcam/WebCam.h")]
public class WebCam
{
	public static extern WebCamMode Mode
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeName("GetWebCamMode")]
		[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
		get;
	}
}

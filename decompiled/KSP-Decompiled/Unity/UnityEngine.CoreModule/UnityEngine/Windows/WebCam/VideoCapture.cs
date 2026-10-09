using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;
using UnityEngine.Scripting;
using UnityEngine.Scripting.APIUpdating;

namespace UnityEngine.Windows.WebCam;

[StructLayout(LayoutKind.Sequential)]
[MovedFrom("UnityEngine.XR.WSA.WebCam")]
[StaticAccessor("VideoCaptureBindings", StaticAccessorType.DoubleColon)]
[NativeHeader("PlatformDependent/Win/Webcam/VideoCaptureBindings.h")]
public class VideoCapture : IDisposable
{
	public enum CaptureResultType
	{
		Success,
		UnknownError
	}

	public enum AudioState
	{
		MicAudio,
		ApplicationAudio,
		ApplicationAndMicAudio,
		None
	}

	public struct VideoCaptureResult
	{
		public CaptureResultType resultType;

		public long hResult;

		public bool success => resultType == CaptureResultType.Success;
	}

	public delegate void OnVideoCaptureResourceCreatedCallback(VideoCapture captureObject);

	public delegate void OnVideoModeStartedCallback(VideoCaptureResult result);

	public delegate void OnVideoModeStoppedCallback(VideoCaptureResult result);

	public delegate void OnStartedRecordingVideoCallback(VideoCaptureResult result);

	public delegate void OnStoppedRecordingVideoCallback(VideoCaptureResult result);

	internal IntPtr m_NativePtr;

	private static Resolution[] s_SupportedResolutions;

	private static readonly long HR_SUCCESS = 0L;

	public static IEnumerable<Resolution> SupportedResolutions
	{
		get
		{
			if (s_SupportedResolutions == null)
			{
				s_SupportedResolutions = GetSupportedResolutions_Internal();
			}
			return s_SupportedResolutions;
		}
	}

	public extern bool IsRecording
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		[NativeMethod("VideoCaptureBindings::IsRecording", HasExplicitThis = true)]
		[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
		get;
	}

	private static VideoCaptureResult MakeCaptureResult(CaptureResultType resultType, long hResult)
	{
		VideoCaptureResult result = default(VideoCaptureResult);
		result.resultType = resultType;
		result.hResult = hResult;
		return result;
	}

	private static VideoCaptureResult MakeCaptureResult(long hResult)
	{
		VideoCaptureResult result = default(VideoCaptureResult);
		CaptureResultType resultType = ((hResult != HR_SUCCESS) ? CaptureResultType.UnknownError : CaptureResultType.Success);
		result.resultType = resultType;
		result.hResult = hResult;
		return result;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[NativeName("GetSupportedResolutions")]
	private static extern Resolution[] GetSupportedResolutions_Internal();

	public static IEnumerable<float> GetSupportedFrameRatesForResolution(Resolution resolution)
	{
		float[] array = null;
		return GetSupportedFrameRatesForResolution_Internal(resolution.width, resolution.height);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeName("GetSupportedFrameRatesForResolution")]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	private static extern float[] GetSupportedFrameRatesForResolution_Internal(int resolutionWidth, int resolutionHeight);

	public static void CreateAsync(bool showHolograms, OnVideoCaptureResourceCreatedCallback onCreatedCallback)
	{
		if (onCreatedCallback == null)
		{
			throw new ArgumentNullException("onCreatedCallback");
		}
		Instantiate_Internal(showHolograms, onCreatedCallback);
	}

	public static void CreateAsync(OnVideoCaptureResourceCreatedCallback onCreatedCallback)
	{
		if (onCreatedCallback == null)
		{
			throw new ArgumentNullException("onCreatedCallback");
		}
		Instantiate_Internal(showHolograms: false, onCreatedCallback);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeName("Instantiate")]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	private static extern void Instantiate_Internal(bool showHolograms, OnVideoCaptureResourceCreatedCallback onCreatedCallback);

	[RequiredByNativeCode]
	private static void InvokeOnCreatedVideoCaptureResourceDelegate(OnVideoCaptureResourceCreatedCallback callback, IntPtr nativePtr)
	{
		if (nativePtr == IntPtr.Zero)
		{
			callback(null);
		}
		else
		{
			callback(new VideoCapture(nativePtr));
		}
	}

	private VideoCapture(IntPtr nativeCaptureObject)
	{
		m_NativePtr = nativeCaptureObject;
	}

	public void StartVideoModeAsync(CameraParameters setupParams, AudioState audioState, OnVideoModeStartedCallback onVideoModeStartedCallback)
	{
		if (onVideoModeStartedCallback == null)
		{
			throw new ArgumentNullException("onVideoModeStartedCallback");
		}
		if (setupParams.cameraResolutionWidth == 0 || setupParams.cameraResolutionHeight == 0)
		{
			throw new ArgumentOutOfRangeException("setupParams", "The camera resolution must be set to a supported resolution.");
		}
		if (setupParams.frameRate == 0f)
		{
			throw new ArgumentOutOfRangeException("setupParams", "The camera frame rate must be set to a supported recording frame rate.");
		}
		StartVideoMode_Internal(setupParams, audioState, onVideoModeStartedCallback);
	}

	[NativeMethod("VideoCaptureBindings::StartVideoMode", HasExplicitThis = true)]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	private void StartVideoMode_Internal(CameraParameters cameraParameters, AudioState audioState, OnVideoModeStartedCallback onVideoModeStartedCallback)
	{
		StartVideoMode_Internal_Injected(ref cameraParameters, audioState, onVideoModeStartedCallback);
	}

	[RequiredByNativeCode]
	private static void InvokeOnVideoModeStartedDelegate(OnVideoModeStartedCallback callback, long hResult)
	{
		callback(MakeCaptureResult(hResult));
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("VideoCaptureBindings::StopVideoMode", HasExplicitThis = true)]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	public extern void StopVideoModeAsync([NotNull] OnVideoModeStoppedCallback onVideoModeStoppedCallback);

	[RequiredByNativeCode]
	private static void InvokeOnVideoModeStoppedDelegate(OnVideoModeStoppedCallback callback, long hResult)
	{
		callback(MakeCaptureResult(hResult));
	}

	public void StartRecordingAsync(string filename, OnStartedRecordingVideoCallback onStartedRecordingVideoCallback)
	{
		if (onStartedRecordingVideoCallback == null)
		{
			throw new ArgumentNullException("onStartedRecordingVideoCallback");
		}
		if (string.IsNullOrEmpty(filename))
		{
			throw new ArgumentNullException("filename");
		}
		filename = filename.Replace("/", "\\");
		string directoryName = Path.GetDirectoryName(filename);
		if (!string.IsNullOrEmpty(directoryName) && !Directory.Exists(directoryName))
		{
			throw new ArgumentException("The specified directory does not exist.", "filename");
		}
		FileInfo fileInfo = new FileInfo(filename);
		if (fileInfo.Exists && fileInfo.IsReadOnly)
		{
			throw new ArgumentException("Cannot write to the file because it is read-only.", "filename");
		}
		StartRecordingVideoToDisk_Internal(filename, onStartedRecordingVideoCallback);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[NativeMethod("VideoCaptureBindings::StartRecordingVideoToDisk", HasExplicitThis = true)]
	private extern void StartRecordingVideoToDisk_Internal(string filename, OnStartedRecordingVideoCallback onStartedRecordingVideoCallback);

	[RequiredByNativeCode]
	private static void InvokeOnStartedRecordingVideoToDiskDelegate(OnStartedRecordingVideoCallback callback, long hResult)
	{
		callback(MakeCaptureResult(hResult));
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[NativeMethod("VideoCaptureBindings::StopRecordingVideoToDisk", HasExplicitThis = true)]
	public extern void StopRecordingAsync([NotNull] OnStoppedRecordingVideoCallback onStoppedRecordingVideoCallback);

	[RequiredByNativeCode]
	private static void InvokeOnStoppedRecordingVideoToDiskDelegate(OnStoppedRecordingVideoCallback callback, long hResult)
	{
		callback(MakeCaptureResult(hResult));
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[ThreadAndSerializationSafe]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[NativeMethod("VideoCaptureBindings::GetUnsafePointerToVideoDeviceController", HasExplicitThis = true)]
	public extern IntPtr GetUnsafePointerToVideoDeviceController();

	public void Dispose()
	{
		if (m_NativePtr != IntPtr.Zero)
		{
			Dispose_Internal();
			m_NativePtr = IntPtr.Zero;
		}
		GC.SuppressFinalize(this);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("VideoCaptureBindings::Dispose", HasExplicitThis = true)]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	private extern void Dispose_Internal();

	~VideoCapture()
	{
		if (m_NativePtr != IntPtr.Zero)
		{
			DisposeThreaded_Internal();
			m_NativePtr = IntPtr.Zero;
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("VideoCaptureBindings::DisposeThreaded", HasExplicitThis = true)]
	[ThreadAndSerializationSafe]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	private extern void DisposeThreaded_Internal();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void StartVideoMode_Internal_Injected(ref CameraParameters cameraParameters, AudioState audioState, OnVideoModeStartedCallback onVideoModeStartedCallback);
}

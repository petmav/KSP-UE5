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
[StaticAccessor("PhotoCapture", StaticAccessorType.DoubleColon)]
[NativeHeader("PlatformDependent/Win/Webcam/PhotoCapture.h")]
public class PhotoCapture : IDisposable
{
	public enum CaptureResultType
	{
		Success,
		UnknownError
	}

	public struct PhotoCaptureResult
	{
		public CaptureResultType resultType;

		public long hResult;

		public bool success => resultType == CaptureResultType.Success;
	}

	public delegate void OnCaptureResourceCreatedCallback(PhotoCapture captureObject);

	public delegate void OnPhotoModeStartedCallback(PhotoCaptureResult result);

	public delegate void OnPhotoModeStoppedCallback(PhotoCaptureResult result);

	public delegate void OnCapturedToDiskCallback(PhotoCaptureResult result);

	public delegate void OnCapturedToMemoryCallback(PhotoCaptureResult result, PhotoCaptureFrame photoCaptureFrame);

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

	private static PhotoCaptureResult MakeCaptureResult(CaptureResultType resultType, long hResult)
	{
		PhotoCaptureResult result = default(PhotoCaptureResult);
		result.resultType = resultType;
		result.hResult = hResult;
		return result;
	}

	private static PhotoCaptureResult MakeCaptureResult(long hResult)
	{
		PhotoCaptureResult result = default(PhotoCaptureResult);
		CaptureResultType resultType = ((hResult != HR_SUCCESS) ? CaptureResultType.UnknownError : CaptureResultType.Success);
		result.resultType = resultType;
		result.hResult = hResult;
		return result;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[NativeName("GetSupportedResolutions")]
	private static extern Resolution[] GetSupportedResolutions_Internal();

	public static void CreateAsync(bool showHolograms, OnCaptureResourceCreatedCallback onCreatedCallback)
	{
		if (onCreatedCallback == null)
		{
			throw new ArgumentNullException("onCreatedCallback");
		}
		Instantiate_Internal(showHolograms, onCreatedCallback);
	}

	public static void CreateAsync(OnCaptureResourceCreatedCallback onCreatedCallback)
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
	private static extern IntPtr Instantiate_Internal(bool showHolograms, OnCaptureResourceCreatedCallback onCreatedCallback);

	[RequiredByNativeCode]
	private static void InvokeOnCreatedResourceDelegate(OnCaptureResourceCreatedCallback callback, IntPtr nativePtr)
	{
		if (nativePtr == IntPtr.Zero)
		{
			callback(null);
		}
		else
		{
			callback(new PhotoCapture(nativePtr));
		}
	}

	private PhotoCapture(IntPtr nativeCaptureObject)
	{
		m_NativePtr = nativeCaptureObject;
	}

	public void StartPhotoModeAsync(CameraParameters setupParams, OnPhotoModeStartedCallback onPhotoModeStartedCallback)
	{
		if (onPhotoModeStartedCallback == null)
		{
			throw new ArgumentException("onPhotoModeStartedCallback");
		}
		if (setupParams.cameraResolutionWidth == 0 || setupParams.cameraResolutionHeight == 0)
		{
			throw new ArgumentOutOfRangeException("setupParams", "The camera resolution must be set to a supported resolution.");
		}
		StartPhotoMode_Internal(setupParams, onPhotoModeStartedCallback);
	}

	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[NativeName("StartPhotoMode")]
	private void StartPhotoMode_Internal(CameraParameters setupParams, OnPhotoModeStartedCallback onPhotoModeStartedCallback)
	{
		StartPhotoMode_Internal_Injected(ref setupParams, onPhotoModeStartedCallback);
	}

	[RequiredByNativeCode]
	private static void InvokeOnPhotoModeStartedDelegate(OnPhotoModeStartedCallback callback, long hResult)
	{
		callback(MakeCaptureResult(hResult));
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[NativeName("StopPhotoMode")]
	public extern void StopPhotoModeAsync(OnPhotoModeStoppedCallback onPhotoModeStoppedCallback);

	[RequiredByNativeCode]
	private static void InvokeOnPhotoModeStoppedDelegate(OnPhotoModeStoppedCallback callback, long hResult)
	{
		callback(MakeCaptureResult(hResult));
	}

	public void TakePhotoAsync(string filename, PhotoCaptureFileOutputFormat fileOutputFormat, OnCapturedToDiskCallback onCapturedPhotoToDiskCallback)
	{
		if (onCapturedPhotoToDiskCallback == null)
		{
			throw new ArgumentNullException("onCapturedPhotoToDiskCallback");
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
		CapturePhotoToDisk_Internal(filename, fileOutputFormat, onCapturedPhotoToDiskCallback);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeName("CapturePhotoToDisk")]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	private extern void CapturePhotoToDisk_Internal(string filename, PhotoCaptureFileOutputFormat fileOutputFormat, OnCapturedToDiskCallback onCapturedPhotoToDiskCallback);

	[RequiredByNativeCode]
	private static void InvokeOnCapturedPhotoToDiskDelegate(OnCapturedToDiskCallback callback, long hResult)
	{
		callback(MakeCaptureResult(hResult));
	}

	public void TakePhotoAsync(OnCapturedToMemoryCallback onCapturedPhotoToMemoryCallback)
	{
		if (onCapturedPhotoToMemoryCallback == null)
		{
			throw new ArgumentNullException("onCapturedPhotoToMemoryCallback");
		}
		CapturePhotoToMemory_Internal(onCapturedPhotoToMemoryCallback);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[NativeName("CapturePhotoToMemory")]
	private extern void CapturePhotoToMemory_Internal(OnCapturedToMemoryCallback onCapturedPhotoToMemoryCallback);

	[RequiredByNativeCode]
	private static void InvokeOnCapturedPhotoToMemoryDelegate(OnCapturedToMemoryCallback callback, long hResult, IntPtr photoCaptureFramePtr)
	{
		PhotoCaptureFrame photoCaptureFrame = null;
		if (photoCaptureFramePtr != IntPtr.Zero)
		{
			photoCaptureFrame = new PhotoCaptureFrame(photoCaptureFramePtr);
		}
		callback(MakeCaptureResult(hResult), photoCaptureFrame);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[ThreadAndSerializationSafe]
	[NativeName("GetUnsafePointerToVideoDeviceController")]
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
	[NativeName("Dispose")]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	private extern void Dispose_Internal();

	~PhotoCapture()
	{
		if (m_NativePtr != IntPtr.Zero)
		{
			DisposeThreaded_Internal();
			m_NativePtr = IntPtr.Zero;
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[ThreadAndSerializationSafe]
	[NativeName("DisposeThreaded")]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	private extern void DisposeThreaded_Internal();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void StartPhotoMode_Internal_Injected(ref CameraParameters setupParams, OnPhotoModeStartedCallback onPhotoModeStartedCallback);
}

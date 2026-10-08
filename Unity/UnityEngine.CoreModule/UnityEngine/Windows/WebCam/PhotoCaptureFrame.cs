using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;
using UnityEngine.Scripting.APIUpdating;

namespace UnityEngine.Windows.WebCam;

[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
[MovedFrom("UnityEngine.XR.WSA.WebCam")]
[NativeHeader("PlatformDependent/Win/Webcam/PhotoCaptureFrame.h")]
public sealed class PhotoCaptureFrame : IDisposable
{
	private IntPtr m_NativePtr;

	public int dataLength { get; private set; }

	public bool hasLocationData { get; private set; }

	public CapturePixelFormat pixelFormat { get; private set; }

	[MethodImpl(MethodImplOptions.InternalCall)]
	[ThreadAndSerializationSafe]
	private extern int GetDataLength();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[ThreadAndSerializationSafe]
	private extern bool GetHasLocationData();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[ThreadAndSerializationSafe]
	private extern CapturePixelFormat GetCapturePixelFormat();

	public bool TryGetCameraToWorldMatrix(out Matrix4x4 cameraToWorldMatrix)
	{
		cameraToWorldMatrix = Matrix4x4.identity;
		if (hasLocationData)
		{
			cameraToWorldMatrix = GetCameraToWorldMatrix();
			return true;
		}
		return false;
	}

	[NativeConditional("PLATFORM_WIN && !PLATFORM_XBOXONE", "Matrix4x4f()")]
	[ThreadAndSerializationSafe]
	[NativeName("GetCameraToWorld")]
	private Matrix4x4 GetCameraToWorldMatrix()
	{
		GetCameraToWorldMatrix_Injected(out var ret);
		return ret;
	}

	public bool TryGetProjectionMatrix(out Matrix4x4 projectionMatrix)
	{
		if (hasLocationData)
		{
			projectionMatrix = GetProjection();
			return true;
		}
		projectionMatrix = Matrix4x4.identity;
		return false;
	}

	public bool TryGetProjectionMatrix(float nearClipPlane, float farClipPlane, out Matrix4x4 projectionMatrix)
	{
		if (hasLocationData)
		{
			float num = 0.01f;
			if (nearClipPlane < num)
			{
				nearClipPlane = num;
			}
			if (farClipPlane < nearClipPlane + num)
			{
				farClipPlane = nearClipPlane + num;
			}
			projectionMatrix = GetProjection();
			float num2 = 1f / (farClipPlane - nearClipPlane);
			float m = (0f - (farClipPlane + nearClipPlane)) * num2;
			float m2 = (0f - 2f * farClipPlane * nearClipPlane) * num2;
			projectionMatrix.m22 = m;
			projectionMatrix.m23 = m2;
			return true;
		}
		projectionMatrix = Matrix4x4.identity;
		return false;
	}

	[NativeConditional("PLATFORM_WIN && !PLATFORM_XBOXONE", "Matrix4x4f()")]
	[ThreadAndSerializationSafe]
	private Matrix4x4 GetProjection()
	{
		GetProjection_Injected(out var ret);
		return ret;
	}

	public void UploadImageDataToTexture(Texture2D targetTexture)
	{
		if (targetTexture == null)
		{
			throw new ArgumentNullException("targetTexture");
		}
		if (pixelFormat != 0)
		{
			throw new ArgumentException("Uploading PhotoCaptureFrame to a texture is only supported with BGRA32 CameraFrameFormat!");
		}
		UploadImageDataToTexture_Internal(targetTexture);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeName("UploadImageDataToTexture")]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[ThreadAndSerializationSafe]
	private extern void UploadImageDataToTexture_Internal(Texture2D targetTexture);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[ThreadAndSerializationSafe]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	public extern IntPtr GetUnsafePointerToBuffer();

	public void CopyRawImageDataIntoBuffer(List<byte> byteBuffer)
	{
		if (byteBuffer == null)
		{
			throw new ArgumentNullException("byteBuffer");
		}
		byte[] array = new byte[dataLength];
		CopyRawImageDataIntoBuffer_Internal(array);
		if (byteBuffer.Capacity < array.Length)
		{
			byteBuffer.Capacity = array.Length;
		}
		byteBuffer.Clear();
		byteBuffer.AddRange(array);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeName("CopyRawImageDataIntoBuffer")]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	[ThreadAndSerializationSafe]
	internal extern void CopyRawImageDataIntoBuffer_Internal([Out] byte[] byteArray);

	internal PhotoCaptureFrame(IntPtr nativePtr)
	{
		m_NativePtr = nativePtr;
		dataLength = GetDataLength();
		hasLocationData = GetHasLocationData();
		pixelFormat = GetCapturePixelFormat();
		GC.AddMemoryPressure(dataLength);
	}

	private void Cleanup()
	{
		if (m_NativePtr != IntPtr.Zero)
		{
			GC.RemoveMemoryPressure(dataLength);
			Dispose_Internal();
			m_NativePtr = IntPtr.Zero;
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[ThreadAndSerializationSafe]
	[NativeName("Dispose")]
	[NativeConditional("(PLATFORM_WIN || PLATFORM_WINRT) && !PLATFORM_XBOXONE")]
	private extern void Dispose_Internal();

	public void Dispose()
	{
		Cleanup();
		GC.SuppressFinalize(this);
	}

	~PhotoCaptureFrame()
	{
		Cleanup();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void GetCameraToWorldMatrix_Injected(out Matrix4x4 ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void GetProjection_Injected(out Matrix4x4 ret);
}

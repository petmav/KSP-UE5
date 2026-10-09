using System.Linq;
using UnityEngine.Bindings;
using UnityEngine.Scripting;
using UnityEngine.Scripting.APIUpdating;

namespace UnityEngine.Windows.WebCam;

[NativeHeader("PlatformDependent/Win/Webcam/CameraParameters.h")]
[UsedByNativeCode]
[MovedFrom("UnityEngine.XR.WSA.WebCam")]
public struct CameraParameters
{
	private float m_HologramOpacity;

	private float m_FrameRate;

	private int m_CameraResolutionWidth;

	private int m_CameraResolutionHeight;

	private CapturePixelFormat m_PixelFormat;

	public float hologramOpacity
	{
		get
		{
			return m_HologramOpacity;
		}
		set
		{
			m_HologramOpacity = value;
		}
	}

	public float frameRate
	{
		get
		{
			return m_FrameRate;
		}
		set
		{
			m_FrameRate = value;
		}
	}

	public int cameraResolutionWidth
	{
		get
		{
			return m_CameraResolutionWidth;
		}
		set
		{
			m_CameraResolutionWidth = value;
		}
	}

	public int cameraResolutionHeight
	{
		get
		{
			return m_CameraResolutionHeight;
		}
		set
		{
			m_CameraResolutionHeight = value;
		}
	}

	public CapturePixelFormat pixelFormat
	{
		get
		{
			return m_PixelFormat;
		}
		set
		{
			m_PixelFormat = value;
		}
	}

	public CameraParameters(WebCamMode webCamMode)
	{
		m_HologramOpacity = 1f;
		m_PixelFormat = CapturePixelFormat.BGRA32;
		m_FrameRate = 0f;
		m_CameraResolutionWidth = 0;
		m_CameraResolutionHeight = 0;
		switch (webCamMode)
		{
		case WebCamMode.PhotoMode:
		{
			Resolution resolution2 = PhotoCapture.SupportedResolutions.OrderByDescending((Resolution res) => res.width * res.height).First();
			m_CameraResolutionWidth = resolution2.width;
			m_CameraResolutionHeight = resolution2.height;
			break;
		}
		case WebCamMode.VideoMode:
		{
			Resolution resolution = VideoCapture.SupportedResolutions.OrderByDescending((Resolution res) => res.width * res.height).First();
			float num = (from fps in VideoCapture.GetSupportedFrameRatesForResolution(resolution)
				orderby fps descending
				select fps).First();
			m_CameraResolutionWidth = resolution.width;
			m_CameraResolutionHeight = resolution.height;
			m_FrameRate = num;
			break;
		}
		}
	}
}

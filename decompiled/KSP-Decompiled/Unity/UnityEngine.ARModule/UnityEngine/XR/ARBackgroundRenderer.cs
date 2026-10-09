using System;
using UnityEngine.Rendering;

namespace UnityEngine.XR;

public class ARBackgroundRenderer
{
	protected Camera m_Camera = null;

	protected Material m_BackgroundMaterial = null;

	protected Texture m_BackgroundTexture = null;

	private ARRenderMode m_RenderMode = ARRenderMode.StandardBackground;

	private CommandBuffer m_CommandBuffer = null;

	private CameraClearFlags m_CameraClearFlags = CameraClearFlags.Skybox;

	public Material backgroundMaterial
	{
		get
		{
			return m_BackgroundMaterial;
		}
		set
		{
			if (!(m_BackgroundMaterial == value))
			{
				RemoveCommandBuffersIfNeeded();
				m_BackgroundMaterial = value;
				if (this.backgroundRendererChanged != null)
				{
					this.backgroundRendererChanged();
				}
				ReapplyCommandBuffersIfNeeded();
			}
		}
	}

	public Texture backgroundTexture
	{
		get
		{
			return m_BackgroundTexture;
		}
		set
		{
			if (!(m_BackgroundTexture = value))
			{
				RemoveCommandBuffersIfNeeded();
				m_BackgroundTexture = value;
				if (this.backgroundRendererChanged != null)
				{
					this.backgroundRendererChanged();
				}
				ReapplyCommandBuffersIfNeeded();
			}
		}
	}

	public Camera camera
	{
		get
		{
			return (m_Camera != null) ? m_Camera : Camera.main;
		}
		set
		{
			if (!(m_Camera == value))
			{
				RemoveCommandBuffersIfNeeded();
				m_Camera = value;
				if (this.backgroundRendererChanged != null)
				{
					this.backgroundRendererChanged();
				}
				ReapplyCommandBuffersIfNeeded();
			}
		}
	}

	public ARRenderMode mode
	{
		get
		{
			return m_RenderMode;
		}
		set
		{
			if (value != m_RenderMode)
			{
				m_RenderMode = value;
				switch (m_RenderMode)
				{
				case ARRenderMode.StandardBackground:
					DisableARBackgroundRendering();
					break;
				case ARRenderMode.MaterialAsBackground:
					EnableARBackgroundRendering();
					break;
				default:
					throw new Exception("Unhandled render mode.");
				}
				if (this.backgroundRendererChanged != null)
				{
					this.backgroundRendererChanged();
				}
			}
		}
	}

	public event Action backgroundRendererChanged = null;

	protected bool EnableARBackgroundRendering()
	{
		if (m_BackgroundMaterial == null)
		{
			return false;
		}
		Camera camera = ((!(m_Camera != null)) ? Camera.main : m_Camera);
		if (camera == null)
		{
			return false;
		}
		m_CameraClearFlags = camera.clearFlags;
		camera.clearFlags = CameraClearFlags.Depth;
		m_CommandBuffer = new CommandBuffer();
		Texture texture = m_BackgroundTexture;
		if (texture == null && m_BackgroundMaterial.HasProperty("_MainTex"))
		{
			texture = m_BackgroundMaterial.GetTexture("_MainTex");
		}
		m_CommandBuffer.Blit(texture, BuiltinRenderTextureType.CameraTarget, m_BackgroundMaterial);
		camera.AddCommandBuffer(CameraEvent.BeforeForwardOpaque, m_CommandBuffer);
		camera.AddCommandBuffer(CameraEvent.BeforeGBuffer, m_CommandBuffer);
		return true;
	}

	protected void DisableARBackgroundRendering()
	{
		if (m_CommandBuffer != null)
		{
			Camera camera = m_Camera ?? Camera.main;
			if (!(camera == null))
			{
				camera.clearFlags = m_CameraClearFlags;
				camera.RemoveCommandBuffer(CameraEvent.BeforeForwardOpaque, m_CommandBuffer);
				camera.RemoveCommandBuffer(CameraEvent.BeforeGBuffer, m_CommandBuffer);
			}
		}
	}

	private bool ReapplyCommandBuffersIfNeeded()
	{
		if (m_RenderMode != ARRenderMode.MaterialAsBackground)
		{
			return false;
		}
		EnableARBackgroundRendering();
		return true;
	}

	private bool RemoveCommandBuffersIfNeeded()
	{
		if (m_RenderMode != ARRenderMode.MaterialAsBackground)
		{
			return false;
		}
		DisableARBackgroundRendering();
		return true;
	}
}

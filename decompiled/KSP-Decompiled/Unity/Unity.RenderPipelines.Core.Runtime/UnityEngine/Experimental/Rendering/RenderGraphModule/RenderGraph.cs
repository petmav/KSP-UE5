using System;
using System.Collections.Generic;
using UnityEngine.Rendering;

namespace UnityEngine.Experimental.Rendering.RenderGraphModule;

public class RenderGraph
{
	internal abstract class RenderPass
	{
		internal string name;

		internal int index;

		internal ProfilingSampler customSampler;

		internal List<RenderGraphResource> resourceReadList = new List<RenderGraphResource>();

		internal List<RenderGraphMutableResource> resourceWriteList = new List<RenderGraphMutableResource>();

		internal List<RenderGraphResource> usedRendererListList = new List<RenderGraphResource>();

		internal bool enableAsyncCompute;

		protected RenderGraphMutableResource[] m_ColorBuffers = new RenderGraphMutableResource[kMaxMRTCount];

		protected RenderGraphMutableResource m_DepthBuffer;

		protected int m_MaxColorBufferIndex = -1;

		internal RenderGraphMutableResource depthBuffer => m_DepthBuffer;

		internal RenderGraphMutableResource[] colorBuffers => m_ColorBuffers;

		internal int colorBufferMaxIndex => m_MaxColorBufferIndex;

		internal RenderFunc<PassData> GetExecuteDelegate<PassData>() where PassData : class, new()
		{
			return ((RenderPass<PassData>)this).renderFunc;
		}

		internal abstract void Execute(RenderGraphContext renderGraphContext);

		internal abstract void Release(RenderGraphContext renderGraphContext);

		internal abstract bool HasRenderFunc();

		internal void Clear()
		{
			name = "";
			index = -1;
			customSampler = null;
			resourceReadList.Clear();
			resourceWriteList.Clear();
			usedRendererListList.Clear();
			enableAsyncCompute = false;
			m_MaxColorBufferIndex = -1;
			m_DepthBuffer = default(RenderGraphMutableResource);
			for (int i = 0; i < kMaxMRTCount; i++)
			{
				m_ColorBuffers[i] = default(RenderGraphMutableResource);
			}
		}

		internal void SetColorBuffer(in RenderGraphMutableResource resource, int index)
		{
			m_MaxColorBufferIndex = Math.Max(m_MaxColorBufferIndex, index);
			m_ColorBuffers[index] = resource;
			resourceWriteList.Add(resource);
		}

		internal void SetDepthBuffer(in RenderGraphMutableResource resource, DepthAccess flags)
		{
			m_DepthBuffer = resource;
			if ((flags | DepthAccess.Read) != 0)
			{
				resourceReadList.Add(resource);
			}
			if ((flags | DepthAccess.Write) != 0)
			{
				resourceWriteList.Add(resource);
			}
		}
	}

	internal sealed class RenderPass<PassData> : RenderPass where PassData : class, new()
	{
		internal PassData data;

		internal RenderFunc<PassData> renderFunc;

		internal override void Execute(RenderGraphContext renderGraphContext)
		{
			GetExecuteDelegate<PassData>()(data, renderGraphContext);
		}

		internal override void Release(RenderGraphContext renderGraphContext)
		{
			Clear();
			renderGraphContext.renderGraphPool.Release(data);
			data = null;
			renderFunc = null;
			renderGraphContext.renderGraphPool.Release(this);
		}

		internal override bool HasRenderFunc()
		{
			return renderFunc != null;
		}
	}

	public static readonly int kMaxMRTCount = 8;

	private RenderGraphResourceRegistry m_Resources;

	private RenderGraphObjectPool m_RenderGraphPool = new RenderGraphObjectPool();

	private List<RenderPass> m_RenderPasses = new List<RenderPass>();

	private List<RenderGraphResource> m_RendererLists = new List<RenderGraphResource>();

	private RenderGraphDebugParams m_DebugParameters = new RenderGraphDebugParams();

	private RenderGraphLogger m_Logger = new RenderGraphLogger();

	public bool enabled => m_DebugParameters.enableRenderGraph;

	public RTHandleProperties rtHandleProperties => m_Resources.GetRTHandleProperties();

	public RenderGraph(bool supportMSAA, MSAASamples initialSampleCount)
	{
		m_Resources = new RenderGraphResourceRegistry(supportMSAA, initialSampleCount, m_DebugParameters, m_Logger);
	}

	public void Cleanup()
	{
		m_Resources.Cleanup();
	}

	public void RegisterDebug()
	{
	}

	public void UnRegisterDebug()
	{
	}

	public RenderGraphMutableResource ImportTexture(RTHandle rt, int shaderProperty = 0)
	{
		return m_Resources.ImportTexture(rt, shaderProperty);
	}

	public RenderGraphMutableResource CreateTexture(TextureDesc desc, int shaderProperty = 0)
	{
		if (m_DebugParameters.tagResourceNamesWithRG)
		{
			desc.name = $"{desc.name}_RenderGraph";
		}
		return m_Resources.CreateTexture(in desc, shaderProperty);
	}

	public RenderGraphMutableResource CreateTexture(in RenderGraphResource texture, int shaderProperty = 0)
	{
		TextureDesc desc = m_Resources.GetTextureResourceDesc(texture);
		if (m_DebugParameters.tagResourceNamesWithRG)
		{
			desc.name = $"{desc.name}_RenderGraph";
		}
		return m_Resources.CreateTexture(in desc, shaderProperty);
	}

	public TextureDesc GetTextureDesc(in RenderGraphResource texture)
	{
		if (texture.type != RenderGraphResourceType.Texture)
		{
			throw new ArgumentException("Trying to retrieve a TextureDesc from a resource that is not a texture.");
		}
		return m_Resources.GetTextureResourceDesc(texture);
	}

	public RenderGraphResource CreateRendererList(in RendererListDesc desc)
	{
		return m_Resources.CreateRendererList(in desc);
	}

	public RenderGraphBuilder AddRenderPass<PassData>(string passName, out PassData passData, ProfilingSampler sampler = null) where PassData : class, new()
	{
		RenderPass<PassData> renderPass = m_RenderGraphPool.Get<RenderPass<PassData>>();
		renderPass.Clear();
		renderPass.index = m_RenderPasses.Count;
		renderPass.data = m_RenderGraphPool.Get<PassData>();
		renderPass.name = passName;
		renderPass.customSampler = sampler;
		passData = renderPass.data;
		m_RenderPasses.Add(renderPass);
		return new RenderGraphBuilder(renderPass, m_Resources);
	}

	public void Execute(ScriptableRenderContext renderContext, CommandBuffer cmd, in RenderGraphExecuteParams parameters)
	{
		m_Logger.Initialize();
		m_Resources.SetRTHandleReferenceSize(parameters.renderingWidth, parameters.renderingHeight, parameters.msaaSamples);
		LogFrameInformation(parameters.renderingWidth, parameters.renderingHeight);
		for (int i = 0; i < m_RenderPasses.Count; i++)
		{
			RenderPass renderPass = m_RenderPasses[i];
			m_RendererLists.AddRange(renderPass.usedRendererListList);
		}
		m_Resources.CreateRendererLists(m_RendererLists);
		LogRendererListsCreation();
		RenderGraphContext renderGraphContext = default(RenderGraphContext);
		renderGraphContext.cmd = cmd;
		renderGraphContext.renderContext = renderContext;
		renderGraphContext.renderGraphPool = m_RenderGraphPool;
		renderGraphContext.resources = m_Resources;
		try
		{
			for (int j = 0; j < m_RenderPasses.Count; j++)
			{
				RenderPass pass = m_RenderPasses[j];
				if (!pass.HasRenderFunc())
				{
					throw new InvalidOperationException($"RenderPass {pass.name} was not provided with an execute function.");
				}
				using (new ProfilingScope(cmd, pass.customSampler))
				{
					LogRenderPassBegin(in pass);
					using (new RenderGraphLogIndent(m_Logger))
					{
						PreRenderPassExecute(j, in pass, renderGraphContext);
						pass.Execute(renderGraphContext);
						PostRenderPassExecute(j, in pass, renderGraphContext);
					}
				}
			}
		}
		catch (Exception exception)
		{
			Debug.LogError("Render Graph Execution error");
			Debug.LogException(exception);
		}
		finally
		{
			ClearRenderPasses();
			m_Resources.Clear();
			m_RendererLists.Clear();
			if (m_DebugParameters.logFrameInformation || m_DebugParameters.logResources)
			{
				Debug.Log(m_Logger.GetLog());
			}
			m_DebugParameters.logFrameInformation = false;
			m_DebugParameters.logResources = false;
		}
	}

	private RenderGraph()
	{
	}

	private void PreRenderPassSetRenderTargets(in RenderPass pass, RenderGraphContext rgContext)
	{
		if (!pass.depthBuffer.IsValid() && pass.colorBufferMaxIndex == -1)
		{
			return;
		}
		RenderTargetIdentifier[] tempArray = rgContext.renderGraphPool.GetTempArray<RenderTargetIdentifier>(pass.colorBufferMaxIndex + 1);
		RenderGraphMutableResource[] colorBuffers = pass.colorBuffers;
		if (pass.colorBufferMaxIndex > 0)
		{
			RenderGraphResource handle;
			for (int i = 0; i <= pass.colorBufferMaxIndex; i++)
			{
				if (!colorBuffers[i].IsValid())
				{
					throw new InvalidOperationException("MRT setup is invalid. Some indices are not used.");
				}
				int num = i;
				RenderGraphResourceRegistry resources = m_Resources;
				handle = colorBuffers[i];
				tempArray[num] = resources.GetTexture(in handle);
			}
			if (!pass.depthBuffer.IsValid())
			{
				throw new InvalidOperationException("Setting MRTs without a depth buffer is not supported.");
			}
			CommandBuffer cmd = rgContext.cmd;
			RenderGraphResourceRegistry resources2 = m_Resources;
			handle = pass.depthBuffer;
			CoreUtils.SetRenderTarget(cmd, tempArray, resources2.GetTexture(in handle));
		}
		else if (pass.depthBuffer.IsValid())
		{
			if (pass.colorBufferMaxIndex > -1)
			{
				CommandBuffer cmd2 = rgContext.cmd;
				RenderGraphResourceRegistry resources3 = m_Resources;
				RenderGraphResource handle = pass.colorBuffers[0];
				RTHandle texture = resources3.GetTexture(in handle);
				RenderGraphResourceRegistry resources4 = m_Resources;
				RenderGraphResource handle2 = pass.depthBuffer;
				CoreUtils.SetRenderTarget(cmd2, texture, resources4.GetTexture(in handle2));
			}
			else
			{
				CommandBuffer cmd3 = rgContext.cmd;
				RenderGraphResourceRegistry resources5 = m_Resources;
				RenderGraphResource handle = pass.depthBuffer;
				CoreUtils.SetRenderTarget(cmd3, resources5.GetTexture(in handle));
			}
		}
		else
		{
			CommandBuffer cmd4 = rgContext.cmd;
			RenderGraphResourceRegistry resources6 = m_Resources;
			RenderGraphResource handle = pass.colorBuffers[0];
			CoreUtils.SetRenderTarget(cmd4, resources6.GetTexture(in handle));
		}
	}

	private void PreRenderPassExecute(int passIndex, in RenderPass pass, RenderGraphContext rgContext)
	{
		m_Resources.CreateAndClearTexturesForPass(rgContext, pass.index, pass.resourceWriteList);
		PreRenderPassSetRenderTargets(in pass, rgContext);
		m_Resources.PreRenderPassSetGlobalTextures(rgContext, pass.resourceReadList);
	}

	private void PostRenderPassExecute(int passIndex, in RenderPass pass, RenderGraphContext rgContext)
	{
		if (m_DebugParameters.unbindGlobalTextures)
		{
			m_Resources.PostRenderPassUnbindGlobalTextures(rgContext, pass.resourceReadList);
		}
		m_RenderGraphPool.ReleaseAllTempAlloc();
		m_Resources.ReleaseTexturesForPass(rgContext, pass.index, pass.resourceReadList, pass.resourceWriteList);
		pass.Release(rgContext);
	}

	private void ClearRenderPasses()
	{
		m_RenderPasses.Clear();
	}

	private void LogFrameInformation(int renderingWidth, int renderingHeight)
	{
		if (m_DebugParameters.logFrameInformation)
		{
			m_Logger.LogLine("==== Staring frame at resolution ({0}x{1}) ====", renderingWidth, renderingHeight);
			m_Logger.LogLine("Number of passes declared: {0}", m_RenderPasses.Count);
		}
	}

	private void LogRendererListsCreation()
	{
		if (m_DebugParameters.logFrameInformation)
		{
			m_Logger.LogLine("Number of renderer lists created: {0}", m_RendererLists.Count);
		}
	}

	private void LogRenderPassBegin(in RenderPass pass)
	{
		if (m_DebugParameters.logFrameInformation)
		{
			m_Logger.LogLine("Executing pass \"{0}\" (index: {1})", pass.name, pass.index);
		}
	}
}

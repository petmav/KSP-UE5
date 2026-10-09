using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.SceneManagement;

namespace Highlighting
{
	[RequireComponent(typeof(Camera))]
	public class HighlightingSystem : MonoBehaviour
	{
		private static bool ppfxEnabled = true;

		protected static readonly Color colorClear = new Color(0f, 0f, 0f, 0f);

		protected static readonly string renderBufferName = "HighlightingSystem";

		protected static readonly Matrix4x4 identityMatrix = Matrix4x4.identity;

		protected const CameraEvent queue = CameraEvent.BeforeImageEffectsOpaque;

		protected static RenderTargetIdentifier cameraTargetID;

		protected static Mesh quad;

		protected const int OGL = 0;

		protected const int D3D9 = 1;

		protected const int D3D11 = 2;

		protected static int graphicsDeviceVersion = 1;

		public float offsetFactor;

		public float offsetUnits;

		protected CommandBuffer renderBuffer;

		protected bool isDirty = true;

		protected int cachedWidth = -1;

		protected int cachedHeight = -1;

		protected int cachedAA = -1;

		[SerializeField]
		protected int _downsampleFactor = 4;

		[SerializeField]
		protected int _iterations = 2;

		[SerializeField]
		protected float _blurMinSpread = 0.65f;

		[SerializeField]
		protected float _blurSpread = 0.25f;

		[SerializeField]
		protected float _blurIntensity = 0.3f;

		protected RenderTargetIdentifier highlightingBufferID;

		protected RenderTexture highlightingBuffer;

		protected Camera cam;

		protected bool isSupported;

		protected bool isDepthAvailable = true;

		protected const int BLUR = 0;

		protected const int CUT = 1;

		protected const int COMP = 2;

		protected static readonly string[] shaderPaths = new string[3] { "Hidden/Highlighted/Blur", "Hidden/Highlighted/Cut", "Hidden/Highlighted/Composite" };

		protected static Shader[] shaders;

		protected static Material[] materials;

		protected static Material cutMaterial;

		protected static Material compMaterial;

		protected Material blurMaterial;

		protected static bool initialized = false;

		public static bool FxEnabled
		{
			get
			{
				return ppfxEnabled;
			}
			set
			{
				ppfxEnabled = value;
				if (ppfxEnabled)
				{
					HighlightingSystem[] array = Object.FindObjectsOfType<HighlightingSystem>();
					int num = array.Length;
					while (num-- > 0)
					{
						array[num].enabled = true;
					}
				}
			}
		}

		public int downsampleFactor
		{
			get
			{
				return _downsampleFactor;
			}
			set
			{
				if (_downsampleFactor != value)
				{
					if (value != 0 && (value & (value - 1)) == 0)
					{
						_downsampleFactor = value;
						isDirty = true;
					}
					else
					{
						Debug.LogWarning("HighlightingSystem : Prevented attempt to set incorrect downsample factor value.");
					}
				}
			}
		}

		public int iterations
		{
			get
			{
				return _iterations;
			}
			set
			{
				if (_iterations != value)
				{
					_iterations = value;
					isDirty = true;
				}
			}
		}

		public float blurMinSpread
		{
			get
			{
				return _blurMinSpread;
			}
			set
			{
				if (_blurMinSpread != value)
				{
					_blurMinSpread = value;
					isDirty = true;
				}
			}
		}

		public float blurSpread
		{
			get
			{
				return _blurSpread;
			}
			set
			{
				if (_blurSpread != value)
				{
					_blurSpread = value;
					isDirty = true;
				}
			}
		}

		public float blurIntensity
		{
			get
			{
				return _blurIntensity;
			}
			set
			{
				if (_blurIntensity != value)
				{
					_blurIntensity = value;
					if (Application.isPlaying)
					{
						blurMaterial.SetFloat(Highlighter._Intensity, _blurIntensity);
					}
				}
			}
		}

		internal bool IsDirty
		{
			get
			{
				return isDirty;
			}
			set
			{
				isDirty = value;
			}
		}

		public bool IsSupported => isSupported;

		public bool IsDepthAvailable => isDepthAvailable;

		protected virtual void Awake()
		{
			SceneManager.sceneLoaded += OnSceneLoaded;
			SceneManager.sceneUnloaded += OnSceneUnloaded;
		}

		protected virtual void OnDestroy()
		{
			SceneManager.sceneLoaded -= OnSceneLoaded;
			SceneManager.sceneUnloaded -= OnSceneUnloaded;
		}

		protected virtual void OnSceneUnloaded(Scene scene)
		{
			base.enabled = false;
		}

		protected virtual void OnSceneLoaded(Scene scene, LoadSceneMode mode)
		{
			if (GameSettings.HIGHLIGHT_FX && (HighLogic.LoadedSceneIsFlight || HighLogic.LoadedSceneIsEditor))
			{
				if (base.gameObject.activeInHierarchy)
				{
					StartCoroutine(RefreshCoroutine());
				}
			}
			else
			{
				base.enabled = false;
			}
		}

		protected virtual IEnumerator RefreshCoroutine(int n = 10)
		{
			while (n-- > 0)
			{
				yield return null;
			}
			base.enabled = false;
			base.enabled = true;
		}

		public virtual void Cycle()
		{
			OnDisable();
			OnEnable();
		}

		public virtual void Disable()
		{
			OnDisable();
		}

		protected virtual void OnEnable()
		{
			Initialize();
			isSupported = CheckSupported();
			if (isSupported && GameSettings.HIGHLIGHT_FX && (HighLogic.LoadedSceneIsFlight || HighLogic.LoadedSceneIsEditor) && (!EVAConstructionModeController.Instance || !EVAConstructionModeController.Instance.IsOpen || EVAConstructionModeController.Instance.panelMode != 0))
			{
				ppfxEnabled = true;
				blurMaterial = new Material(materials[0]);
				blurMaterial.SetFloat(Highlighter._Intensity, _blurIntensity);
				renderBuffer = new CommandBuffer();
				renderBuffer.name = renderBufferName;
				cam = GetComponent<Camera>();
				UpdateHighlightingBuffer();
				isDirty = true;
				base.enabled = true;
				cam.AddCommandBuffer(CameraEvent.BeforeImageEffectsOpaque, renderBuffer);
			}
			else
			{
				base.enabled = false;
				ppfxEnabled = false;
			}
		}

		protected virtual void OnDisable()
		{
			if (renderBuffer != null)
			{
				cam.RemoveCommandBuffer(CameraEvent.BeforeImageEffectsOpaque, renderBuffer);
				renderBuffer = null;
			}
			if (highlightingBuffer != null && highlightingBuffer.IsCreated())
			{
				highlightingBuffer.Release();
				highlightingBuffer = null;
			}
			base.enabled = false;
		}

		protected virtual void OnPreRender()
		{
			UpdateHighlightingBuffer();
			int aA = GetAA();
			bool flag = aA == 1;
			if (aA > 1 && (cam.actualRenderingPath == RenderingPath.Forward || cam.actualRenderingPath == RenderingPath.VertexLit))
			{
				flag = false;
			}
			if (isDepthAvailable != flag)
			{
				isDepthAvailable = flag;
				Highlighter.SetZWrite(isDepthAvailable ? 0f : 1f);
				if (isDepthAvailable)
				{
					Debug.LogWarning("HighlightingSystem : Framebuffer depth data is available back again and will be used to occlude highlighting. Highlighting occluders disabled.");
				}
				else
				{
					Debug.LogWarning("HighlightingSystem : Framebuffer depth data is not available and can't be used to occlude highlighting. Highlighting occluders enabled.");
				}
				isDirty = true;
			}
			Highlighter.SetOffsetFactor(offsetFactor);
			Highlighter.SetOffsetUnits(offsetUnits);
			isDirty |= Highlighter.HighlightDirty;
			isDirty |= HighlightersChanged();
			if (isDirty)
			{
				RebuildCommandBuffer();
				isDirty = false;
			}
		}

		protected virtual void OnRenderImage(RenderTexture src, RenderTexture dst)
		{
			Graphics.Blit(src, dst, compMaterial);
		}

		protected static void Initialize()
		{
			if (!initialized)
			{
				string text = SystemInfo.graphicsDeviceVersion.ToLower();
				if (!text.Contains("direct3d") && !text.Contains("directx"))
				{
					graphicsDeviceVersion = 0;
				}
				else if (!text.Contains("direct3d 11") && !text.Contains("directx 11"))
				{
					graphicsDeviceVersion = 1;
				}
				else
				{
					graphicsDeviceVersion = 2;
				}
				Highlighter.Initialize();
				int num = shaderPaths.Length;
				shaders = new Shader[num];
				materials = new Material[num];
				for (int i = 0; i < num; i++)
				{
					Shader shader = Shader.Find(shaderPaths[i]);
					shaders[i] = shader;
					Material material = new Material(shader);
					materials[i] = material;
				}
				cutMaterial = materials[1];
				compMaterial = materials[2];
				cameraTargetID = new RenderTargetIdentifier(BuiltinRenderTextureType.CameraTarget);
				CreateQuad();
				initialized = true;
			}
		}

		protected static void CreateQuad()
		{
			if (quad == null)
			{
				quad = new Mesh();
			}
			else
			{
				quad.Clear();
			}
			float y = 1f;
			float y2 = -1f;
			if (graphicsDeviceVersion == 0)
			{
				y = -1f;
				y2 = 1f;
			}
			quad.vertices = new Vector3[4]
			{
				new Vector3(-1f, y, 0f),
				new Vector3(-1f, y2, 0f),
				new Vector3(1f, y2, 0f),
				new Vector3(1f, y, 0f)
			};
			quad.uv = new Vector2[4]
			{
				new Vector2(0f, 0f),
				new Vector2(0f, 1f),
				new Vector2(1f, 1f),
				new Vector2(1f, 0f)
			};
			quad.colors = new Color[4] { colorClear, colorClear, colorClear, colorClear };
			quad.triangles = new int[6] { 0, 1, 2, 2, 3, 0 };
		}

		protected virtual int GetAA()
		{
			int num = QualitySettings.antiAliasing;
			if (num == 0)
			{
				num = 1;
			}
			if (cam.actualRenderingPath == RenderingPath.DeferredLighting || cam.actualRenderingPath == RenderingPath.DeferredShading)
			{
				num = 1;
			}
			return num;
		}

		protected virtual void UpdateHighlightingBuffer()
		{
			int aA = GetAA();
			if (cam.pixelWidth != cachedWidth || cam.pixelHeight != cachedHeight || aA != cachedAA || !(highlightingBuffer != null))
			{
				cachedWidth = cam.pixelWidth;
				cachedHeight = cam.pixelHeight;
				cachedAA = aA;
				if (highlightingBuffer != null && highlightingBuffer.IsCreated())
				{
					highlightingBuffer.Release();
				}
				highlightingBuffer = new RenderTexture(cachedWidth, cachedHeight, 24, RenderTextureFormat.ARGB32, RenderTextureReadWrite.Default);
				highlightingBuffer.antiAliasing = cachedAA;
				highlightingBuffer.filterMode = FilterMode.Point;
				highlightingBuffer.useMipMap = false;
				highlightingBuffer.wrapMode = TextureWrapMode.Clamp;
				if (!highlightingBuffer.Create())
				{
					Debug.LogError("HighlightingSystem : UpdateHighlightingBuffer() : Failed to create highlightingBuffer RenderTexture!");
				}
				highlightingBufferID = new RenderTargetIdentifier(highlightingBuffer);
				Shader.SetGlobalTexture(Highlighter._HighlightingBuffer, highlightingBuffer);
				Vector4 value = new Vector4(((graphicsDeviceVersion == 0) ? 1f : (-1f)) / (float)highlightingBuffer.width, 1f / (float)highlightingBuffer.height, 0f, 0f);
				Shader.SetGlobalVector(Highlighter._HighlightingBufferTexelSize, value);
				isDirty = true;
			}
		}

		public virtual bool CheckInstance()
		{
			HighlightingSystem[] components = GetComponents<HighlightingSystem>();
			if (components.Length > 1 && components[0] != this)
			{
				base.enabled = false;
				string arg = GetType().ToString();
				Debug.LogWarning($"HighlightingSystem : Only single instance of the HighlightingRenderer component is allowed on a single Gameobject! {arg} has been disabled on GameObject with name '{base.name}'.");
				return false;
			}
			return true;
		}

		public static bool CheckSupported()
		{
			Initialize();
			if (!SystemInfo.supportsImageEffects)
			{
				Debug.LogWarning("HighlightingSystem : Image effects is not supported on this platform!");
				return false;
			}
			if (!SystemInfo.SupportsRenderTextureFormat(RenderTextureFormat.ARGB32))
			{
				Debug.LogWarning("HighlightingSystem : RenderTextureFormat.ARGB32 is not supported on this platform!");
				return false;
			}
			if (!Highlighter.opaqueShader.isSupported)
			{
				Debug.LogWarning("HighlightingSystem : HighlightingOpaque shader is not supported on this platform!");
				return false;
			}
			if (!Highlighter.transparentShader.isSupported)
			{
				Debug.LogWarning("HighlightingSystem : HighlightingTransparent shader is not supported on this platform!");
				return false;
			}
			int num = 0;
			Shader shader;
			while (true)
			{
				if (num < shaders.Length)
				{
					shader = shaders[num];
					if (!shader.isSupported)
					{
						break;
					}
					num++;
					continue;
				}
				if (QualitySettings.antiAliasing == 0)
				{
					Debug.LogWarning("HighlightingSystem : Edge Highlighting requires AA to work!");
					return false;
				}
				return true;
			}
			Debug.LogWarning("HighlightingSystem : Shader '" + shader.name + "' is not supported on this platform!");
			return false;
		}

		protected virtual bool HighlightersChanged()
		{
			bool flag = false;
			HashSet<Highlighter>.Enumerator enumerator = Highlighter.GetEnumerator();
			while (enumerator.MoveNext())
			{
				Highlighter current = enumerator.Current;
				flag |= current.UpdateHighlighting(isDepthAvailable);
			}
			return flag;
		}

		protected virtual void RebuildCommandBuffer()
		{
			renderBuffer.Clear();
			RenderTargetIdentifier depth = (isDepthAvailable ? cameraTargetID : highlightingBufferID);
			renderBuffer.SetRenderTarget(highlightingBufferID, depth);
			renderBuffer.ClearRenderTarget(!isDepthAvailable, clearColor: true, colorClear);
			FillBuffer(renderBuffer, 0);
			FillBuffer(renderBuffer, 1);
			FillBuffer(renderBuffer, 2);
			RenderTargetIdentifier renderTargetIdentifier = new RenderTargetIdentifier(Highlighter._HighlightingBlur1);
			RenderTargetIdentifier renderTargetIdentifier2 = new RenderTargetIdentifier(Highlighter._HighlightingBlur2);
			int width = highlightingBuffer.width / _downsampleFactor;
			int height = highlightingBuffer.height / _downsampleFactor;
			renderBuffer.GetTemporaryRT(Highlighter._HighlightingBlur1, width, height, 0, FilterMode.Bilinear, RenderTextureFormat.ARGB32, RenderTextureReadWrite.Default);
			renderBuffer.GetTemporaryRT(Highlighter._HighlightingBlur2, width, height, 0, FilterMode.Bilinear, RenderTextureFormat.ARGB32, RenderTextureReadWrite.Default);
			renderBuffer.Blit(highlightingBufferID, renderTargetIdentifier);
			bool flag = true;
			for (int i = 0; i < _iterations; i++)
			{
				float value = _blurMinSpread + _blurSpread * (float)i;
				renderBuffer.SetGlobalFloat(Highlighter._HighlightingBlurOffset, value);
				if (flag)
				{
					renderBuffer.Blit(renderTargetIdentifier, renderTargetIdentifier2, blurMaterial);
				}
				else
				{
					renderBuffer.Blit(renderTargetIdentifier2, renderTargetIdentifier, blurMaterial);
				}
				flag = !flag;
			}
			renderBuffer.SetGlobalTexture(Highlighter._HighlightingBlurred, flag ? renderTargetIdentifier : renderTargetIdentifier2);
			renderBuffer.SetRenderTarget(highlightingBufferID, depth);
			renderBuffer.DrawMesh(quad, identityMatrix, cutMaterial);
			renderBuffer.ReleaseTemporaryRT(Highlighter._HighlightingBlur1);
			renderBuffer.ReleaseTemporaryRT(Highlighter._HighlightingBlur2);
		}

		protected virtual void FillBuffer(CommandBuffer buffer, int renderQueue)
		{
			HashSet<Highlighter>.Enumerator enumerator = Highlighter.GetEnumerator();
			while (enumerator.MoveNext())
			{
				enumerator.Current.FillBuffer(renderBuffer, renderQueue);
			}
		}

		public void ReloadSettings()
		{
			isSupported = CheckSupported();
		}
	}
}

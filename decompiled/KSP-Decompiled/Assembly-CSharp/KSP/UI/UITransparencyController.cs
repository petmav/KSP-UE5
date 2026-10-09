using System;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace KSP.UI
{
	public class UITransparencyController : MonoBehaviour
	{
		[Serializable]
		public class ShaderFader
		{
			[SerializeField]
			private Material mat;

			[SerializeField]
			private string colorPropertyName = string.Empty;

			[SerializeField]
			private string alphaPropertyName = string.Empty;

			[SerializeField]
			private bool absoluteAlpha = true;

			private int propID;

			private bool useColorProp;

			private Color baseColor;

			private float baseAlpha;

			private bool init;

			public bool Init => init;

			public bool Setup()
			{
				if (mat != null)
				{
					useColorProp = !string.IsNullOrEmpty(colorPropertyName);
					if (useColorProp)
					{
						propID = Shader.PropertyToID(colorPropertyName);
						baseColor = mat.GetColor(propID);
					}
					else
					{
						propID = Shader.PropertyToID(alphaPropertyName);
						baseAlpha = mat.GetFloat(propID);
					}
				}
				init = true;
				return init;
			}

			public void SetOpacity(float opacity)
			{
				if (!init)
				{
					return;
				}
				if (useColorProp)
				{
					if (absoluteAlpha)
					{
						mat.SetColor(propID, baseColor.A(opacity));
					}
					else
					{
						mat.SetColor(propID, new Color(baseColor.r, baseColor.g, baseColor.b, baseColor.a * opacity));
					}
				}
				else if (absoluteAlpha)
				{
					mat.SetFloat(propID, opacity);
				}
				else
				{
					mat.SetFloat(propID, baseAlpha * opacity);
				}
			}
		}

		[SerializeField]
		private float opacity;

		[SerializeField]
		private CanvasGroup[] canvasGroups;

		[SerializeField]
		private ShaderFader[] shaderFaders;

		public float Opacity => opacity;

		private void Start()
		{
			GameEvents.OnGameSettingsApplied.Add(onSettingsApplied);
			SceneManager.sceneLoaded += OnSceneLoaded;
		}

		private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
		{
			OnLevelLoaded(HighLogic.GetLoadedGameSceneFromBuildIndex(scene.buildIndex));
		}

		private void OnLevelLoaded(GameScenes lvl)
		{
			if (HighLogic.LoadedSceneIsFlight)
			{
				UpdateOpacity(GameSettings.UI_OPACITY);
			}
			else
			{
				UpdateOpacity(1f);
			}
		}

		private void OnDestroy()
		{
			GameEvents.OnGameSettingsApplied.Remove(onSettingsApplied);
			SceneManager.sceneLoaded -= OnSceneLoaded;
		}

		private void onSettingsApplied()
		{
			UpdateOpacity(GameSettings.UI_OPACITY);
		}

		public void UpdateOpacity(float opacity)
		{
			int num = canvasGroups.Length;
			while (num-- > 0)
			{
				canvasGroups[num].alpha = opacity;
			}
			int num2 = shaderFaders.Length;
			while (num2-- > 0)
			{
				ShaderFader shaderFader = shaderFaders[num2];
				if (!shaderFader.Init)
				{
					shaderFader.Setup();
					shaderFader.SetOpacity(opacity);
				}
				else
				{
					shaderFader.SetOpacity(opacity);
				}
			}
			this.opacity = opacity;
		}
	}
}

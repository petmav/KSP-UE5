using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using UnityEngine;

namespace KSPAssets.Loaders
{
	public class CodeAssetLoader : MonoBehaviour
	{
		private List<Assembly> assemblies = new List<Assembly>();

		private bool ready = false;

		private AssetLoader.Loader loader;

		public static CodeAssetLoader Instance { get; private set; }

		public bool Ready => ready;

		private void Awake()
		{
			if (Instance != null)
			{
				Debug.Log("CodeAssetLoader: Instance already exists. Destroying potential usurper", base.gameObject);
				Object.Destroy(this);
			}
			else
			{
				Instance = this;
			}
		}

		private IEnumerator Start()
		{
			ready = false;
			while (!AssetLoader.Ready)
			{
				yield return null;
			}
			CompileAll();
		}

		public bool CompileAll()
		{
			Debug.Log("CodeAssetLoader: Compiling all code assets");
			ready = false;
			if (!AssetLoader.Ready)
			{
				return false;
			}
			StartCoroutine(CompileAllAsync());
			return true;
		}

		private IEnumerator CompileAllAsync()
		{
			if (!AssetLoader.LoadAssets(assetDefinitions: AssetLoader.GetAssetDefinitionsWithType("dll"), onFinished: OnAsyncTextAssetLoaded))
			{
				yield break;
			}
			loader = null;
			while (loader == null)
			{
				yield return null;
			}
			int i = 0;
			while (i < loader.definitions.Length)
			{
				AssetDefinition assetDef = loader.definitions[i];
				if (!(loader.objects[i] == null))
				{
					TextAsset textAsset = loader.objects[i] as TextAsset;
					LoadAssembly(assetDef, textAsset.bytes);
				}
				int num = i + 1;
				i = num;
			}
			loader = null;
			ready = true;
		}

		private void OnAsyncTextAssetLoaded(AssetLoader.Loader loader)
		{
			this.loader = loader;
		}

		private void LoadAssembly(AssetDefinition assetDef, byte[] bytes)
		{
			Debug.Log("CodeAssetLoader: Loading code asset '" + assetDef.name + "'");
			Assembly assembly = Assembly.Load(bytes);
			if (assembly != null)
			{
				assemblies.Add(assembly);
			}
		}
	}
}

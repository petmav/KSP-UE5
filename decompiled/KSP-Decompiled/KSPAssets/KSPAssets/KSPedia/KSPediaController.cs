using System;
using System.Collections;
using System.Collections.Generic;
using KSPAssets.Loaders;
using UnityEngine;
using UnityEngine.UI;

namespace KSPAssets.KSPedia
{
	public class KSPediaController : MonoBehaviour
	{
		[Serializable]
		protected class BundleWrapper
		{
			public string name;

			public string url;

			public AssetBundle bundle;

			public BundleWrapper(string name, string url)
			{
				this.name = name;
				this.url = url;
			}
		}

		protected List<BundleWrapper> bundles;

		protected KSPediaDatabase db;

		protected Dictionary<string, GameObject> loadedSlides;

		protected string currentActiveSlideName;

		private AssetLoader.Loader loader = null;

		private Coroutine loadScreenRoutine = null;

		public static KSPediaController Instance { get; private set; }

		private void Awake()
		{
			if (Instance != null)
			{
				Debug.LogError("KSPediaController: Instance already exists - destroying second", base.gameObject);
				UnityEngine.Object.Destroy(this);
			}
			else
			{
				Instance = this;
			}
		}

		private IEnumerator Start()
		{
			OnStart();
			while (!AssetLoader.Ready)
			{
				yield return null;
			}
			db = new KSPediaDatabase();
			bundles = new List<BundleWrapper>();
			loadedSlides = new Dictionary<string, GameObject>();
			currentActiveSlideName = string.Empty;
			VerifyDependencies();
			yield return StartCoroutine(CompileDatabase());
			OnDatabaseReady();
		}

		private void OnDestroy()
		{
			HideScreen();
			Instance = null;
		}

		private void VerifyDependencies()
		{
			BundleDefinition bundleDefinition = AssetLoader.GetBundleDefinition("Squad/squadcore");
			if (bundleDefinition != null && bundleDefinition.dependencyBundles.Count == 0)
			{
				BundleDefinition bundleDefinition2 = AssetLoader.GetBundleDefinition("Squad/squadcorefx");
				if (bundleDefinition2 != null)
				{
					bundleDefinition.dependencyBundles.Add(bundleDefinition2);
					Debug.LogWarning("KSPediaController: Fix broken dependencies!");
				}
				else
				{
					Debug.LogWarning("KSPediaController: SquadCoreFx bundle is not found!");
				}
			}
			else if (bundleDefinition == null)
			{
				Debug.LogWarning("KSPediaController: SquadCore bundle is not found!");
			}
		}

		private IEnumerator CompileDatabase()
		{
			Debug.Log("KSPediaController: Compiling KSPedia databases");
			List<AssetDefinition> assets = new List<AssetDefinition>();
			int i = 0;
			while (i < AssetLoader.AssetDefinitions.Count)
			{
				AssetDefinition a = AssetLoader.AssetDefinitions[i];
				if (a.type == "TextAsset" && a.name.EndsWith("_kspedia"))
				{
					assets.Add(a);
				}
				int num = i + 1;
				i = num;
			}
			Debug.Log("KSPediaController: Compiling " + assets.Count + " KSPedia databases");
			if (assets.Count > 0)
			{
				AssetLoader.LoadAssets(OnAssetsLoaded, assets.ToArray());
			}
			loader = null;
			while (loader == null)
			{
				yield return null;
			}
			int j = 0;
			while (j < loader.definitions.Length)
			{
				AssetDefinition assetDef = loader.definitions[j];
				if (!(loader.objects[j] == null))
				{
					TextAsset textAsset = loader.objects[j] as TextAsset;
					if (!(textAsset == null))
					{
						Debug.Log("KSPediaController: Merging " + assetDef.name + " database");
						KSPediaDatabase newDB = KSPediaDatabase.CreateFromText(textAsset.text);
						db.Merge(newDB);
					}
				}
				int num = j + 1;
				j = num;
			}
			Debug.Log("KSPediaController: Completed compiling databases");
			loader = null;
		}

		private void OnAssetsLoaded(AssetLoader.Loader loader)
		{
			this.loader = loader;
		}

		public bool ShowScreen(string name)
		{
			bool flag = false;
			if (loadedSlides.ContainsKey(name))
			{
				flag = true;
			}
			if (!flag)
			{
				KSPediaDatabase.Screen screen = db.GetScreen(name);
				if (screen == null)
				{
					return false;
				}
				if (loadScreenRoutine == null)
				{
					loadScreenRoutine = StartCoroutine(LoadScreen(screen));
					return true;
				}
				return false;
			}
			ActivateScreen(name);
			return true;
		}

		private IEnumerator LoadScreen(KSPediaDatabase.Screen dbScreen)
		{
			Debug.Log("KSPediaController: Loading screen " + dbScreen.name);
			AssetDefinition a = AssetLoader.GetAssetDefinitionWithPath(dbScreen.assetPath);
			if (a == null)
			{
				string bName = "kspedia_" + dbScreen.name.ToLower() + ".ksp";
				BundleDefinition bundleDef = null;
				yield return StartCoroutine(AssetLoader.Instance.LoadDefinitionByNameAsync(bName, delegate(BundleDefinition value)
				{
					bundleDef = value;
				}));
				if (bundleDef != null)
				{
					a = AssetLoader.GetAssetDefinitionWithPath(bundleDef, dbScreen.assetPath);
				}
				if (a == null)
				{
					Debug.LogError("[KSPediaController]: Cannot find asset definition with path '" + dbScreen.assetPath + "' for screen '" + dbScreen.name);
					yield break;
				}
			}
			loader = null;
			if (!AssetLoader.LoadAssets(OnScreenLoaded, false, a))
			{
				Debug.LogError("KSPediaController: Asset load request invalid");
				yield break;
			}
			while (loader == null)
			{
				yield return null;
			}
			if (loader.objects[0] == null)
			{
				Debug.LogError("KSPediaController: Asset load failed");
				yield break;
			}
			GameObject screenObject = loader.objects[0] as GameObject;
			if (screenObject == null)
			{
				Debug.LogError("KSPediaController: GameObject cast failed");
				yield break;
			}
			InstantiateScreen(screenObject, dbScreen);
			loadScreenRoutine = null;
		}

		public void ReleaseLoadingCoroutine()
		{
			loadScreenRoutine = null;
		}

		private void OnScreenLoaded(AssetLoader.Loader loader)
		{
			this.loader = loader;
		}

		private void ActivateScreen(string name)
		{
			loadedSlides[name].SetActive(value: true);
			if (currentActiveSlideName != name)
			{
				loadedSlides[currentActiveSlideName].SetActive(value: false);
				currentActiveSlideName = name;
			}
		}

		private GameObject InstantiateScreen(GameObject prefab, KSPediaDatabase.Screen dbScreen)
		{
			GameObject gameObject = UnityEngine.Object.Instantiate(prefab);
			OnInstantiateScreenPrefab(gameObject);
			OnCompileScreen(gameObject, dbScreen);
			OnShowScreen(dbScreen);
			loadedSlides.Add(dbScreen.name, gameObject);
			if (currentActiveSlideName != dbScreen.name)
			{
				if (loadedSlides.ContainsKey(currentActiveSlideName))
				{
					loadedSlides[currentActiveSlideName].SetActive(value: false);
				}
				currentActiveSlideName = dbScreen.name;
			}
			return gameObject;
		}

		protected virtual void OnStart()
		{
		}

		protected virtual void OnDatabaseReady()
		{
		}

		protected virtual void OnShowScreen(KSPediaDatabase.Screen dbScreen)
		{
		}

		protected virtual void OnInstantiateScreenPrefab(GameObject go)
		{
			go.transform.SetParent(base.transform, worldPositionStays: false);
		}

		protected virtual void OnCompileScreen(GameObject go, KSPediaDatabase.Screen dbScreen)
		{
			OnCompileText(go, dbScreen);
			OnCompileTooltips(go, dbScreen);
		}

		protected virtual void OnCompileText(GameObject go, KSPediaDatabase.Screen dbScreen)
		{
			Text[] componentsInChildren = go.GetComponentsInChildren<Text>(includeInactive: true);
			for (int i = 0; i < componentsInChildren.Length; i++)
			{
				string componentURL = KSPediaDatabase.Screen.GetComponentURL(go.transform, componentsInChildren[i].transform);
				KSPediaDatabase.ScreenText text = dbScreen.GetText(componentURL);
				if (text != null)
				{
					componentsInChildren[i].text = text.text;
				}
			}
		}

		protected virtual void OnCompileTooltips(GameObject go, KSPediaDatabase.Screen dbScreen)
		{
			DatabaseTooltip[] componentsInChildren = go.GetComponentsInChildren<DatabaseTooltip>(includeInactive: true);
			for (int i = 0; i < componentsInChildren.Length; i++)
			{
				string componentURL = KSPediaDatabase.Screen.GetComponentURL(go.transform, componentsInChildren[i].transform);
				KSPediaDatabase.ScreenText tooltip = dbScreen.GetTooltip(componentURL);
				if (tooltip != null)
				{
					componentsInChildren[i].text = tooltip.text;
				}
			}
		}

		public void HideScreen()
		{
			OnHideScreen();
		}

		protected virtual void OnHideScreen()
		{
			AssetLoader.UnloadBundles();
		}
	}
}

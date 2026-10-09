using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using KSPAssets.KSPedia;
using UnityEngine;
using UnityEngine.Networking;

namespace KSPAssets.Loaders
{
	public class AssetLoader : MonoBehaviour
	{
		private class FileComparer : IEqualityComparer<FileInfo>
		{
			public bool Equals(FileInfo x, FileInfo y)
			{
				return x.FullName.Equals(y.FullName);
			}

			public int GetHashCode(FileInfo obj)
			{
				return obj.FullName.GetHashCode();
			}
		}

		public class Loader
		{
			public AssetDefinition[] definitions;

			public UnityEngine.Object[] objects;

			public Action<Loader> onFinished;

			public Loader(Action<Loader> onFinished, params AssetDefinition[] assetDefinitions)
			{
				this.onFinished = onFinished;
				definitions = assetDefinitions;
				objects = new UnityEngine.Object[assetDefinitions.Length];
			}
		}

		[SerializeField]
		protected string assetDirectory = "GameData";

		protected string coreDirectory = "Squad";

		protected string coreExpansionsDirectory = "SquadExpansion";

		[SerializeField]
		protected string assetExtension = "ksp";

		[SerializeField]
		protected string assetDefinitionSuffix = "_bundle.xml";

		[SerializeField]
		protected bool loadAtStart = false;

		protected string bundleDependencySuffix = "_bundle.bytes";

		[SerializeField]
		private List<BundleDefinition> coreAndAutoloadDefinitions = new List<BundleDefinition>();

		private List<AssetDefinition> assetDefinitions = new List<AssetDefinition>();

		protected bool ready = false;

		[SerializeField]
		private List<AssetBundle> loadedBundles = new List<AssetBundle>();

		[SerializeField]
		private List<BundleDefinition> loadedBundleDefinitions = new List<BundleDefinition>();

		private List<string> assetBlacklist = new List<string>();

		private List<FileInfo> allFilesList = new List<FileInfo>();

		[SerializeField]
		protected int amountAutoLoadBundles = 0;

		private List<Loader> loaderQueue = new List<Loader>();

		private Coroutine loaderRoutine = null;

		public static AssetLoader Instance { get; private set; }

		public static List<BundleDefinition> CoreAndAutoloadDefinitions => Instance.coreAndAutoloadDefinitions;

		public static List<AssetDefinition> AssetDefinitions => Instance.assetDefinitions;

		public static bool Ready => Instance.ready;

		public static List<AssetBundle> LoadedBundles => Instance.loadedBundles;

		public static List<BundleDefinition> LoadedBundleDefinitions => Instance.loadedBundleDefinitions;

		public static List<string> AssetBlacklist => Instance.assetBlacklist;

		public static string ApplicationRootPath => (Application.platform == RuntimePlatform.OSXPlayer) ? (Application.dataPath + "/../../") : (Application.dataPath + "/../");

		private void Awake()
		{
			if (Instance != null)
			{
				Debug.Log("AssetLoader: Instance already exists. Destroying potential usurper", base.gameObject);
				UnityEngine.Object.Destroy(this);
			}
			else
			{
				Instance = this;
			}
		}

		private void Start()
		{
			if (loadAtStart)
			{
				LoadDefinitions();
			}
		}

		public void LoadDefinitions()
		{
			Debug.Log("AssetLoader: Loading bundle definitions");
			StartCoroutine(LoadDefinitionsAsync());
		}

		public IEnumerator LoadDefinitionsAsync()
		{
			Debug.Log("AssetLoader: Loading bundle definitions");
			coreAndAutoloadDefinitions = new List<BundleDefinition>();
			ready = false;
			DirectoryInfo coreDir = new DirectoryInfo(Path.Combine(CreateApplicationPath(assetDirectory), coreDirectory));
			FileInfo[] coreFiles = coreDir.GetFiles("*." + assetExtension, SearchOption.AllDirectories);
			new DirectoryInfo(Path.Combine(CreateApplicationPath(assetDirectory), coreExpansionsDirectory));
			FileInfo[] coreExpFiles = coreDir.GetFiles("*." + assetExtension, SearchOption.AllDirectories);
			DirectoryInfo assetDir = new DirectoryInfo(CreateApplicationPath(assetDirectory));
			FileInfo[] assetFiles = assetDir.GetFiles("*." + assetExtension, SearchOption.AllDirectories);
			List<FileInfo> files = new List<FileInfo>(coreFiles.Where((FileInfo f) => !assetBlacklist.Contains(Path.GetFileName(f.FullName))));
			AddAssetFiles(coreExpFiles, files);
			AddAssetFiles(assetFiles, files);
			allFilesList = files;
			int num;
			for (int i = 0; i < allFilesList.Count; num = i + 1, i = num)
			{
				FileInfo assetFile = allFilesList[i];
				BundleDefinition bundleDefinition2 = null;
				using (UnityWebRequest uwr = UnityWebRequestAssetBundle.GetAssetBundle("file://" + assetFile.FullName))
				{
					yield return uwr.SendWebRequest();
					while (!uwr.isDone)
					{
						yield return null;
					}
					if (uwr.isNetworkError || uwr.isHttpError || !string.IsNullOrEmpty(uwr.error))
					{
						Debug.LogError("AssetLoader - WWW error: " + uwr.error);
						break;
					}
					AssetBundle bundle = DownloadHandlerAssetBundle.GetContent(uwr);
					if (bundle == null)
					{
						Debug.LogError("AssetLoader: Bundle is null");
						continue;
					}
					string[] assetNames = bundle.GetAllAssetNames();
					string[] array = assetNames;
					foreach (string name in array)
					{
						if (name.EndsWith(assetDefinitionSuffix))
						{
							AssetBundleRequest dbRequest = bundle.LoadAssetAsync<TextAsset>(name);
							yield return dbRequest;
							TextAsset dbAsset = dbRequest.asset as TextAsset;
							BundleDefinition bundleDef = BundleDefinition.CreateFromText(dbAsset.text);
							if (bundleDef != null)
							{
								bundleDefinition2 = bundleDef;
							}
						}
						else
						{
							if (!name.EndsWith(bundleDependencySuffix))
							{
								continue;
							}
							string platform = "windows";
							if (Application.platform == RuntimePlatform.LinuxPlayer)
							{
								platform = "linux";
							}
							if (name.Contains(platform))
							{
								AssetBundleRequest dbRequest2 = bundle.LoadAssetAsync<TextAsset>(name);
								yield return dbRequest2;
								TextAsset dbAsset2 = dbRequest2.asset as TextAsset;
								string bundleName = Path.GetFileNameWithoutExtension(name);
								string savePath = Path.Combine(Path.GetDirectoryName(assetFile.FullName), bundleName.Remove(bundleName.IndexOf('_')) + ".ksp");
								FileStream fs = File.Open(savePath, FileMode.OpenOrCreate, FileAccess.ReadWrite);
								fs.SetLength(0L);
								fs.Write(dbAsset2.bytes, 0, dbAsset2.bytes.Length);
								fs.Close();
								FileInfo depBundleFile = new FileInfo(savePath);
								if (!allFilesList.Contains(depBundleFile, new FileComparer()))
								{
									allFilesList.Insert(i + 1, depBundleFile);
								}
							}
						}
					}
					bundle.Unload(unloadAllLoadedObjects: true);
				}
				if (bundleDefinition2 != null)
				{
					if (bundleDefinition2.autoLoad || (!bundleDefinition2.name.ToLower().StartsWith("kspedia_") && bundleDefinition2.name.ToLower().Contains("core")))
					{
						bundleDefinition2.path = assetFile.FullName;
						coreAndAutoloadDefinitions.Add(bundleDefinition2);
						amountAutoLoadBundles++;
						Debug.Log("AssetLoader: Loaded bundle '" + bundleDefinition2.name + "'");
					}
					else if (!bundleDefinition2.autoLoad && !bundleDefinition2.name.ToLower().Contains("core"))
					{
						bundleDefinition2.path = assetFile.FullName;
						coreAndAutoloadDefinitions.Add(bundleDefinition2);
						amountAutoLoadBundles++;
						Debug.Log("AssetLoader: Loaded mod bundle '" + bundleDefinition2.name + "'");
					}
				}
				else
				{
					bundleDefinition2 = new BundleDefinition
					{
						name = assetFile.Name,
						path = assetFile.FullName
					};
				}
			}
			CompileBundleDefinitions();
			CreateAssetDefinitionList();
			Debug.Log("AssetLoader: Finished loading. " + coreAndAutoloadDefinitions.Count + " bundle definitions loaded.");
			ready = true;
		}

		public IEnumerator LoadDefinitionByNameAsync(string defName, Action<BundleDefinition> bdAction)
		{
			FileInfo assetFile = null;
			int f = 0;
			while (f < allFilesList.Count)
			{
				if (allFilesList[f].Name == defName)
				{
					assetFile = allFilesList[f];
				}
				int num = f + 1;
				f = num;
			}
			if (assetFile == null)
			{
				yield break;
			}
			BundleDefinition bundleDefinition = null;
			using (UnityWebRequest uwr = UnityWebRequestAssetBundle.GetAssetBundle("file://" + assetFile.FullName))
			{
				yield return uwr.SendWebRequest();
				while (!uwr.isDone)
				{
					yield return null;
				}
				if (uwr.isNetworkError || uwr.isHttpError || !string.IsNullOrEmpty(uwr.error))
				{
					Debug.LogError("AssetLoader - WWW error: " + uwr.error);
					yield break;
				}
				AssetBundle bundle = DownloadHandlerAssetBundle.GetContent(uwr);
				if (bundle == null)
				{
					Debug.LogError("[AssetLoader]: Bundle" + assetFile.Name + " is null");
					yield break;
				}
				Debug.Log("[AssetLoader]: Loaded bundle '" + assetFile.FullName + "'");
				string[] assetNames = bundle.GetAllAssetNames();
				string[] array = assetNames;
				foreach (string name in array)
				{
					if (name.EndsWith(assetDefinitionSuffix))
					{
						AssetBundleRequest dbRequest = bundle.LoadAssetAsync<TextAsset>(name);
						yield return dbRequest;
						TextAsset dbAsset = dbRequest.asset as TextAsset;
						BundleDefinition bundleDef = BundleDefinition.CreateFromText(dbAsset.text);
						if (bundleDef != null)
						{
							bundleDefinition = bundleDef;
							bundleDefinition.path = assetFile.FullName;
							coreAndAutoloadDefinitions.Add(bundleDefinition);
						}
					}
				}
				bdAction(bundleDefinition);
				bundle.Unload(unloadAllLoadedObjects: true);
			}
		}

		private void AddAssetFiles(FileInfo[] assetFiles, List<FileInfo> files)
		{
			for (int i = 0; i < assetFiles.Length; i++)
			{
				bool flag = false;
				string fullName = assetFiles[i].FullName;
				if (assetBlacklist.Contains(Path.GetFileName(fullName)))
				{
					continue;
				}
				for (int j = 0; j < files.Count; j++)
				{
					if (files[j].FullName == fullName)
					{
						flag = true;
						break;
					}
				}
				if (!flag)
				{
					files.Add(assetFiles[i]);
				}
			}
		}

		private void CompileBundleDefinitions()
		{
			DirectoryInfo directoryInfo = new DirectoryInfo(CreateApplicationPath(assetDirectory));
			int num = directoryInfo.FullName.Length + 1;
			int num2 = assetExtension.Length + 1;
			for (int i = 0; i < coreAndAutoloadDefinitions.Count; i++)
			{
				BundleDefinition bundleDefinition = coreAndAutoloadDefinitions[i];
				bundleDefinition.name = bundleDefinition.path.Substring(num, bundleDefinition.path.Length - num2 - num).Replace('\\', '/');
				for (int j = 0; j < bundleDefinition.assets.Count; j++)
				{
					AssetDefinition assetDefinition = bundleDefinition.assets[j];
					assetDefinition.bundle = bundleDefinition;
				}
				bundleDefinition.dependenciesFound = true;
				for (int k = 0; k < bundleDefinition.dependencyNames.Count; k++)
				{
					string text = bundleDefinition.dependencyNames[k];
					BundleDefinition bundleDefinition2 = null;
					for (int l = 0; l < coreAndAutoloadDefinitions.Count; l++)
					{
						if (i != l && coreAndAutoloadDefinitions[l].urlName == text)
						{
							bundleDefinition2 = coreAndAutoloadDefinitions[l];
							break;
						}
					}
					if (bundleDefinition2 == null)
					{
						bundleDefinition.dependenciesFound = false;
					}
					else
					{
						bundleDefinition.dependencyBundles.Add(bundleDefinition2);
					}
				}
			}
		}

		private void OrderBundleDependencies()
		{
			coreAndAutoloadDefinitions.Sort(delegate(BundleDefinition a, BundleDefinition b)
			{
				if (b.dependencyBundles.Contains(a))
				{
					return -1;
				}
				return a.dependencyBundles.Contains(b) ? 1 : 0;
			});
		}

		private void CreateAssetDefinitionList()
		{
			assetDefinitions = new List<AssetDefinition>();
			for (int i = 0; i < coreAndAutoloadDefinitions.Count; i++)
			{
				BundleDefinition bundleDefinition = coreAndAutoloadDefinitions[i];
				for (int j = 0; j < bundleDefinition.assets.Count; j++)
				{
					assetDefinitions.Add(coreAndAutoloadDefinitions[i].assets[j]);
				}
			}
		}

		public void ReleaseKSPediaAssets()
		{
			for (int i = amountAutoLoadBundles; i < coreAndAutoloadDefinitions.Count; i++)
			{
				coreAndAutoloadDefinitions.RemoveAt(i);
			}
			int count = loadedBundleDefinitions.Count;
			while (count-- > 0)
			{
				if (loadedBundleDefinitions[count].name.ToLower().Contains("kspedia_") && !loadedBundleDefinitions[count].name.ToLower().Contains("core"))
				{
					loadedBundleDefinitions.RemoveAt(count);
				}
			}
			KSPediaController.Instance.ReleaseLoadingCoroutine();
		}

		public static BundleDefinition GetBundleDefinition(string bundleName)
		{
			for (int i = 0; i < Instance.coreAndAutoloadDefinitions.Count; i++)
			{
				if (Instance.coreAndAutoloadDefinitions[i].name == bundleName)
				{
					return Instance.coreAndAutoloadDefinitions[i];
				}
			}
			return null;
		}

		public static AssetDefinition GetAssetDefinitionWithName(string bundleName, string assetName)
		{
			return GetAssetDefinitionWithName(GetBundleDefinition(bundleName), assetName);
		}

		public static AssetDefinition GetAssetDefinitionWithName(BundleDefinition bundleDefinition, string assetName)
		{
			if (bundleDefinition == null)
			{
				return null;
			}
			AssetDefinition assetWithName = bundleDefinition.GetAssetWithName(assetName);
			if (assetWithName == null)
			{
				Debug.LogError("AssetLoader: Cannot find asset with name '" + assetName + "' in bundle '" + bundleDefinition.name + "'");
				return null;
			}
			return assetWithName;
		}

		public static AssetDefinition GetAssetDefinitionWithPath(string assetPath)
		{
			BundleDefinition bundleDefinition = null;
			for (int num = Instance.coreAndAutoloadDefinitions.Count - 1; num >= 0; num--)
			{
				bundleDefinition = Instance.coreAndAutoloadDefinitions[num];
				for (int i = 0; i < bundleDefinition.assets.Count; i++)
				{
					if (bundleDefinition.assets[i].path == assetPath)
					{
						return bundleDefinition.assets[i];
					}
				}
			}
			return null;
		}

		public static AssetDefinition GetAssetDefinitionWithPath(string bundleName, string assetPath)
		{
			return GetAssetDefinitionWithPath(GetBundleDefinition(bundleName), assetPath);
		}

		public static AssetDefinition GetAssetDefinitionWithPath(BundleDefinition bundleDefinition, string assetPath)
		{
			if (bundleDefinition == null)
			{
				return null;
			}
			AssetDefinition assetWithPath = bundleDefinition.GetAssetWithPath(assetPath);
			if (assetWithPath == null)
			{
				Debug.LogError("AssetLoader: Cannot find asset with path '" + assetPath + "' in bundle '" + bundleDefinition.name + "'");
				return null;
			}
			return assetWithPath;
		}

		public static AssetDefinition GetAssetDefinitionWithUrl(string assetUrl)
		{
			for (int num = Instance.coreAndAutoloadDefinitions.Count - 1; num >= 0; num--)
			{
				BundleDefinition bundleDefinition = Instance.coreAndAutoloadDefinitions[num];
				for (int i = 0; i < bundleDefinition.assets.Count; i++)
				{
					if (bundleDefinition.assets[i].url == assetUrl)
					{
						return bundleDefinition.assets[i];
					}
				}
			}
			return null;
		}

		public static AssetDefinition GetAssetDefinitionWithUrl<T>(string assetUrl)
		{
			string text = typeof(T).Name;
			for (int num = Instance.coreAndAutoloadDefinitions.Count - 1; num >= 0; num--)
			{
				BundleDefinition bundleDefinition = Instance.coreAndAutoloadDefinitions[num];
				for (int i = 0; i < bundleDefinition.assets.Count; i++)
				{
					if (bundleDefinition.assets[i].url == assetUrl && bundleDefinition.assets[i].type == text)
					{
						return bundleDefinition.assets[i];
					}
				}
			}
			return null;
		}

		public static AssetDefinition GetAssetDefinitionWithUrl(string bundleName, string assetUrl)
		{
			return GetAssetDefinitionWithUrl(GetBundleDefinition(bundleName), assetUrl);
		}

		public static AssetDefinition GetAssetDefinitionWithUrl(BundleDefinition bundleDefinition, string assetUrl)
		{
			if (bundleDefinition == null)
			{
				return null;
			}
			AssetDefinition assetWithPath = bundleDefinition.GetAssetWithPath(assetUrl);
			if (assetWithPath == null)
			{
				Debug.LogError("AssetLoader: Cannot find asset with path '" + assetUrl + "' in bundle '" + bundleDefinition.name + "'");
				return null;
			}
			return assetWithPath;
		}

		public static AssetDefinition[] GetAssetDefinitionsWithType(BundleDefinition bundle, string typeName)
		{
			List<AssetDefinition> list = new List<AssetDefinition>();
			if (bundle != null)
			{
				for (int i = 0; i < bundle.assets.Count; i++)
				{
					AssetDefinition assetDefinition = bundle.assets[i];
					if (assetDefinition.type == typeName)
					{
						list.Add(assetDefinition);
					}
				}
			}
			return list.ToArray();
		}

		public static AssetDefinition[] GetAssetDefinitionsWithType(string bundleName, string typeName)
		{
			return GetAssetDefinitionsWithType(GetBundleDefinition(bundleName), typeName);
		}

		public static AssetDefinition[] GetAssetDefinitionsWithType(string typeName)
		{
			List<AssetDefinition> list = new List<AssetDefinition>();
			for (int i = 0; i < Instance.coreAndAutoloadDefinitions.Count; i++)
			{
				list.AddRange(GetAssetDefinitionsWithType(Instance.coreAndAutoloadDefinitions[i], typeName));
			}
			return list.ToArray();
		}

		public static AssetDefinition[] GetAssetDefinitionsWithType(string bundleName, Type type)
		{
			return GetAssetDefinitionsWithType(bundleName, type.Name);
		}

		public static AssetDefinition[] GetAssetDefinitionsWithType(Type type)
		{
			return GetAssetDefinitionsWithType(type.Name);
		}

		public static bool LoadAssets(Action<Loader> onFinished, params AssetDefinition[] assetDefinitions)
		{
			if (onFinished == null || assetDefinitions == null || assetDefinitions.Length == 0)
			{
				return false;
			}
			Instance.loaderQueue.Add(new Loader(onFinished, assetDefinitions));
			if (Instance.loaderRoutine == null)
			{
				Instance.loaderRoutine = Instance.StartCoroutine(Instance.LoaderRoutine(unload: true));
			}
			return true;
		}

		public static bool LoadAssets(Action<Loader> onFinished, bool unloadBundles, params AssetDefinition[] assetDefinitions)
		{
			if (onFinished == null || assetDefinitions == null || assetDefinitions.Length == 0)
			{
				return false;
			}
			Instance.loaderQueue.Add(new Loader(onFinished, assetDefinitions));
			if (Instance.loaderRoutine == null)
			{
				Instance.loaderRoutine = Instance.StartCoroutine(Instance.LoaderRoutine(unloadBundles));
			}
			return true;
		}

		private IEnumerator LoaderRoutine(bool unload)
		{
			List<AssetBundle> mainAssets = new List<AssetBundle>();
			while (loaderQueue.Count > 0)
			{
				Loader loader = loaderQueue[0];
				loaderQueue.RemoveAt(0);
				int j = 0;
				while (j < loader.definitions.Length)
				{
					AssetDefinition asset = loader.definitions[j];
					CompileBundleDefinitions();
					BundleDefinition bundleDef = asset.bundle;
					if (bundleDef == null)
					{
						Debug.Log("[AssetLoader]: Bundle is null");
					}
					else
					{
						yield return StartCoroutine(LoadDependencies(bundleDef));
						AssetBundle bundle2 = loadedBundles.FirstOrDefault((AssetBundle t) => t.name == bundleDef.urlName);
						if (bundle2 == null)
						{
							using (UnityWebRequest uwr = UnityWebRequestAssetBundle.GetAssetBundle("file://" + bundleDef.path))
							{
								yield return uwr.SendWebRequest();
								while (!uwr.isDone)
								{
									yield return null;
								}
								if (uwr.isNetworkError || uwr.isHttpError || !string.IsNullOrEmpty(uwr.error))
								{
									Debug.LogError("AssetLoader - WWW error: " + uwr.error);
									break;
								}
								bundle2 = DownloadHandlerAssetBundle.GetContent(uwr);
								if (bundle2 == null)
								{
									Debug.LogError("AssetLoader: Bundle is null");
								}
								else
								{
									loadedBundles.Add(bundle2);
									mainAssets.Add(bundle2);
									if (!loadedBundleDefinitions.Contains(bundleDef))
									{
										loadedBundleDefinitions.Add(bundleDef);
									}
									AssetBundleRequest dbRequest = bundle2.LoadAssetAsync(asset.path);
									yield return dbRequest;
									loader.objects[j] = dbRequest.asset;
								}
							}
						}
						else
						{
							loader.objects[j] = bundle2.LoadAsset(asset.path);
						}
					}
					int num = j + 1;
					j = num;
				}
				if (loader.onFinished != null)
				{
					loader.onFinished(loader);
				}
			}
			loaderRoutine = null;
			if (unload)
			{
				UnloadBundles();
				yield break;
			}
			int i = 0;
			while (i < mainAssets.Count)
			{
				if (mainAssets[i] != null)
				{
					LoadedBundles.Remove(mainAssets[i]);
					mainAssets[i].Unload(unloadAllLoadedObjects: false);
				}
				int num = i + 1;
				i = num;
			}
			mainAssets.Clear();
		}

		private IEnumerator LoadDependencies(BundleDefinition bundleDef)
		{
			if (bundleDef.dependencyBundles.Count <= 0)
			{
				yield break;
			}
			int depsToLoad = bundleDef.dependencyBundles.Count;
			int i = 0;
			while (i < bundleDef.dependencyBundles.Count)
			{
				BundleDefinition depDef = bundleDef.dependencyBundles[i];
				if (depDef.dependencyBundles.Count > 0)
				{
					yield return StartCoroutine(LoadDependencies(depDef));
				}
				BundleDefinition loadedDef = loadedBundleDefinitions.FirstOrDefault((BundleDefinition b) => b.name == depDef.name);
				int num;
				if (loadedDef == null)
				{
					using (UnityWebRequest uwr = UnityWebRequestAssetBundle.GetAssetBundle("file://" + depDef.path))
					{
						yield return uwr.SendWebRequest();
						while (!uwr.isDone)
						{
							yield return null;
						}
						if (uwr.isNetworkError || uwr.isHttpError || !string.IsNullOrEmpty(uwr.error))
						{
							Debug.LogError("AssetLoader - WWW error: " + uwr.error);
							break;
						}
						AssetBundle depBundle = DownloadHandlerAssetBundle.GetContent(uwr);
						if (bundleDef == null)
						{
							Debug.LogError("AssetLoader: Dependency bundle is null");
							break;
						}
						loadedBundles.Add(depBundle);
						if (!loadedBundleDefinitions.Contains(depDef))
						{
							loadedBundleDefinitions.Add(depDef);
						}
						num = depsToLoad - 1;
						depsToLoad = num;
					}
				}
				else
				{
					num = depsToLoad - 1;
					depsToLoad = num;
				}
				num = i + 1;
				i = num;
			}
			if (depsToLoad != 0)
			{
				Debug.LogError("AssetLoader: Dependency bundles were not loaded correctly");
			}
		}

		public static void UnloadBundles()
		{
			for (int i = 0; i < Instance.loadedBundles.Count; i++)
			{
				if (Instance.loadedBundles[i] != null)
				{
					Instance.loadedBundles[i].Unload(unloadAllLoadedObjects: false);
				}
			}
			Instance.loadedBundles.Clear();
			Instance.loadedBundleDefinitions.Clear();
		}

		public static string CreateApplicationPath(string relativePath)
		{
			return Path.Combine(ApplicationRootPath, relativePath);
		}
	}
}
